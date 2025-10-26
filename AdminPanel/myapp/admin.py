from django.contrib import admin, messages
from django import forms

from .models import (
    Profile, RuleConfig, MLRule, ThresholdRule, CompositeRule, PatternRule
)
from .grpc_client import send_profiles_to_director
from .expression_parser import RuleParser


class ExpressionModelForm(forms.ModelForm):
    expression_text = forms.CharField(
        label="Выражение",
        widget=forms.Textarea(attrs={'rows': 4, 'cols': 80}),
        help_text="""
        <strong>Примеры выражений:</strong><br>
        <strong>Threshold:</strong> amount > 100, sender_account = 'test', time = night<br>
        <strong>Pattern:</strong> COUNT(amount) > 5, SUM(amount) >= 1000<br>
        <strong>Composite:</strong> amount > 100 and time = night, amount > 1000 or sender_account = 'vip'<br>
        <em>Поддерживаются операторы: >, <, >=, <=, =, !=, AND, OR, NOT</em>
        """
    )

    class Meta:
        exclude = ('expression',)

    def __init__(self, *args, **kwargs):
        self.rule_type = kwargs.pop('rule_type', None)
        super().__init__(*args, **kwargs)

        if self.instance and self.instance.pk:
            self.fields['expression_text'].help_text += "<br><strong>Внимание:</strong> При редактировании необходимо заново ввести выражение."

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
            self.add_error('expression_text', str(e))
        
        return cleaned_data

    def save(self, commit=True):
        instance = super().save(commit=False)
        if hasattr(self, 'parsed_expression'):
            instance.expression = self.parsed_expression
        if commit:
            instance.save()
        return instance


admin.site.register(MLRule)

class BaseRuleAdmin(admin.ModelAdmin):
    form = ExpressionModelForm
    RULE_TYPE = None  

    def get_form(self, request, obj=None, **kwargs):
        form = super().get_form(request, obj, **kwargs)
        
        class DynamicForm(form):
            def __new__(cls, *args, **kwargs):
                kwargs['rule_type'] = self.RULE_TYPE
                return form(*args, **kwargs)
        
        return DynamicForm

@admin.register(ThresholdRule)
class ThresholdRuleAdmin(BaseRuleAdmin):
    RULE_TYPE = "THRESHOLD"

@admin.register(CompositeRule)
class CompositeRuleAdmin(BaseRuleAdmin):
    RULE_TYPE = "COMPOSITE"

@admin.register(PatternRule)
class PatternRuleAdmin(BaseRuleAdmin):
    RULE_TYPE = "PATTERN"
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