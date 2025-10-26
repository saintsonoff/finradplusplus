from django.contrib import admin, messages
from django import forms

from .models import (
    Profile, RuleConfig, MLRule, ThresholdRule, CompositeRule, PatternRule
)
from .grpc_client import send_profiles_to_director
from .expression_parser import RuleParser


class ExpressionModelForm(forms.ModelForm):
    # Класс-атрибут для хранения типа правила
    RULE_TYPE = None
    
    expression_text = forms.CharField(
        label="Выражение",
        widget=forms.Textarea(attrs={
            'rows': 6, 
            'cols': 80,
            'placeholder': 'Введите выражение здесь...'
        }),
        help_text="""
        <strong>Справка по выражениям:</strong><br>
        <strong>Доступные поля:</strong> TRANSACTION_ID, SENDER_ACCOUNT, RECEIVER_ACCOUNT, AMOUNT, TIMESTAMP, 
        TRANSACTION_TYPE, MERCHANT_CATEGORY, LOCATION, DEVICE_USED, PAYMENT_CHANNEL, IP_ADDRESS, DEVICE_HASH, TIME<br>
        <strong>Операторы сравнения:</strong> >, <, >=, <=, =, ==, !=<br>
        <strong>Логические операторы:</strong> AND, OR, NOT<br>
        <strong>Агрегатные функции:</strong> COUNT, SUM, AVG, MIN, MAX, COUNT_DISTINCT<br>
        <strong>Примеры:</strong><br>
        - THRESHOLD: <code>AMOUNT > 1000</code><br>
        - PATTERN: <code>COUNT(AMOUNT) > 5</code><br>
        - COMPOSITE: <code>AMOUNT > 100 AND TIME = 'night'</code><br>
        <em>Все названия полей должны быть в ВЕРХНЕМ регистре!</em>
        """
    )

    class Meta:
        exclude = ('expression',)

    def __init__(self, *args, **kwargs):
        # Получаем rule_type из kwargs, по умолчанию пытаемся определить из класса
        self.rule_type = kwargs.pop('rule_type', None)
        
        # Если rule_type не передан, пытаемся определить из класса формы
        if self.rule_type is None:
            # Проверяем, есть ли rule_type в классе формы
            if hasattr(self.__class__, 'RULE_TYPE') and self.__class__.RULE_TYPE:
                self.rule_type = self.__class__.RULE_TYPE
        
        super().__init__(*args, **kwargs)

        if self.instance and self.instance.pk:
            self.fields['expression_text'].help_text += "<br><strong> Внимание:</strong> При редактировании необходимо заново ввести выражение."
        
        # Добавляем тип правила в help_text
        if self.rule_type:
            type_info = {
                'THRESHOLD': '🔹 THRESHOLD: Простое сравнение полей (без AND/OR/COUNT)',
                'PATTERN': '🔹 PATTERN: Агрегатные функции (COUNT, SUM и т.д.)',
                'COMPOSITE': '🔹 COMPOSITE: Комплексные выражения с AND/OR',
            }
            info = type_info.get(self.rule_type, f'Тип правила: {self.rule_type}')
            self.fields['expression_text'].help_text = f"<div style='background:#f0f0f0;padding:8px;border-left:4px solid #007cba;margin:8px 0;'><strong>{info}</strong></div>" + self.fields['expression_text'].help_text

    def clean(self):
        cleaned_data = super().clean()
        expression_str = cleaned_data.get("expression_text")

        if not expression_str:
            self.add_error('expression_text', "Это поле не может быть пустым.")
            return cleaned_data

        if not self.rule_type:
            raise forms.ValidationError("Внутренняя ошибка: тип правила не определен.")
        
        try:
            parser = RuleParser()
            self.parsed_expression = parser.parse(expression_str, self.rule_type)
        except ValueError as e:
            error_message = str(e)
            # Улучшаем форматирование ошибок
            if '\n' in error_message:
                parts = error_message.split('\n', 1)
                self.add_error('expression_text', f"{parts[0]}")
                if len(parts) > 1:
                    self.add_error('expression_text', f"{parts[1]}")
            else:
                self.add_error('expression_text', error_message)
        
        return cleaned_data

    def save(self, commit=True):
        instance = super().save(commit=False)
        if hasattr(self, 'parsed_expression'):
            instance.expression = self.parsed_expression
        if commit:
            instance.save()
        return instance


class MLRuleModelForm(forms.ModelForm):
    class Meta:
        model = MLRule
        fields = '__all__'
    
    def clean_model_uuid(self):
        model_uuid = self.cleaned_data.get('model_uuid', '')
        if not model_uuid or not model_uuid.strip():
            raise forms.ValidationError('Model UUID не может быть пустым.')
        return model_uuid.strip()
    
    def clean_lower_bound(self):
        lower_bound = self.cleaned_data.get('lower_bound', 0.0)
        if lower_bound < 0 or lower_bound > 1:
            raise forms.ValidationError('Lower bound должен быть между 0.0 и 1.0.')
        return lower_bound

@admin.register(MLRule)
class MLRuleAdmin(admin.ModelAdmin):
    list_display = ('model_uuid', 'lower_bound')
    fields = ('model_uuid', 'lower_bound')
    form = MLRuleModelForm

class BaseRuleAdmin(admin.ModelAdmin):
    form = ExpressionModelForm
    RULE_TYPE = None  

    def get_form(self, request, obj=None, **kwargs):
        # Получаем RULE_TYPE из текущего экземпляра admin
        rule_type = self.RULE_TYPE
        
        if not rule_type:
            raise ValueError(f"RULE_TYPE is not set in {self.__class__.__name__}")
        
        # Используем form из дочернего класса, если он переопределен
        base_form = self.form
        
        # Создаем форму с привязкой rule_type
        class DynamicForm(base_form):
            RULE_TYPE = rule_type  # Устанавливаем как класс-атрибут
            
            def __init__(self, *args, **kwargs):
                # Передаем rule_type в kwargs, если не передан
                if 'rule_type' not in kwargs or kwargs.get('rule_type') is None:
                    kwargs['rule_type'] = rule_type
                super().__init__(*args, **kwargs)
        
        DynamicForm.__name__ = base_form.__name__
        DynamicForm.__module__ = base_form.__module__
        
        # Заменяем поле form в kwargs
        kwargs['form'] = DynamicForm
        return super().get_form(request, obj, **kwargs)

@admin.register(ThresholdRule)
class ThresholdRuleAdmin(BaseRuleAdmin):
    RULE_TYPE = "THRESHOLD"

@admin.register(CompositeRule)
class CompositeRuleAdmin(BaseRuleAdmin):
    RULE_TYPE = "COMPOSITE"

class PatternRuleModelForm(ExpressionModelForm):
    max_delta_time = forms.IntegerField(
        label="Максимальный промежуток времени (секунды)",
        min_value=1,
        help_text="Максимальный временной промежуток для поиска паттерна в секундах"
    )
    max_count = forms.IntegerField(
        label="Максимальное количество",
        min_value=1,
        help_text="Максимальное количество транзакций, соответствующих паттерну"
    )
    
    class Meta(ExpressionModelForm.Meta):
        model = PatternRule
        fields = ('max_delta_time', 'max_count', 'expression_text')
    
    def __init__(self, *args, **kwargs):
        # По умолчанию для PATTERN правил
        if 'rule_type' not in kwargs:
            kwargs['rule_type'] = 'PATTERN'
        super().__init__(*args, **kwargs)

@admin.register(PatternRule)
class PatternRuleAdmin(BaseRuleAdmin):
    RULE_TYPE = "PATTERN"
    form = PatternRuleModelForm
    fields = ('max_delta_time', 'max_count', 'expression_text')


@admin.register(Profile)
class ProfileAdmin(admin.ModelAdmin):
    list_display = ('name', 'uuid')
    readonly_fields = ('uuid',)
    actions = ['send_selected_profiles']

    @admin.action(description='Отправить выбранные профили в Director')
    def send_selected_profiles(self, request, queryset):
        if not queryset.exists():
            self.message_user(request, "Не выбрано ни одного профиля.", level=messages.WARNING)
            return

        success, message = send_profiles_to_director(queryset)

        if success:
            self.message_user(request, f"Успех! {queryset.count()} профилей отправлено. {message}", level=messages.SUCCESS)
        else:
            self.message_user(request, f"Ошибка отправки. {message}", level=messages.ERROR)


@admin.register(RuleConfig)
class RuleConfigAdmin(admin.ModelAdmin):
    related_lookup_fields = { 'generic': [['content_type', 'object_id']] }
    list_display = ('name', 'profile', 'rule_type', 'is_critical', 'rule', 'uuid')
    readonly_fields = ('uuid',)
    list_filter = ('rule_type', 'is_critical', 'profile')