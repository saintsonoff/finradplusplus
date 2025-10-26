import uuid
from django.db import models
from django.contrib.contenttypes.fields import GenericForeignKey
from django.contrib.contenttypes.models import ContentType

class Profile(models.Model):
    uuid = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=255)

    def __str__(self):
        return self.name

class MLRule(models.Model):
    model_uuid = models.CharField(max_length=255,default='')
    lower_bound = models.FloatField()

    def __str__(self):
        return f"MLRule ({self.model_uuid})"

class ThresholdRule(models.Model):
    expression = models.JSONField()

    def __str__(self):
        return str(self.expression)

class CompositeRule(models.Model):
    expression = models.JSONField()

    def __str__(self):
        return str(self.expression)

class PatternRule(models.Model):
    max_delta_time = models.IntegerField()
    max_count = models.IntegerField()
    expression = models.JSONField()

    def __str__(self):
        return str(self.expression)

class RuleConfig(models.Model):
    class RuleType(models.TextChoices):
        ML = 'ML', 'ML'
        THRESHOLD = 'THRESHOLD', 'THRESHOLD'
        COMPOSITE = 'COMPOSITE', 'COMPOSITE'
        PATTERN = 'PATTERN', 'PATTERN'

    uuid = models.UUIDField(primary_key=True, default=uuid.uuid4, editable=False)
    name = models.CharField(max_length=255)
    rule_type = models.CharField(max_length=20, choices=RuleType.choices)
    is_critical = models.BooleanField(default=False)
    
    profile = models.ForeignKey(Profile, related_name='rules', on_delete=models.CASCADE)

    content_type = models.ForeignKey(ContentType, on_delete=models.CASCADE)
    object_id = models.PositiveIntegerField()
    rule = GenericForeignKey('content_type', 'object_id')

    def __str__(self):
        return self.name

class RuleResult(models.Model):
    class StatusEnum(models.TextChoices):
        ERROR = 'ERROR', 'Error'
        NOT_FRAUD = 'NOT_FRAUD', 'Not Fraud'
        FRAUD = 'FRAUD', 'Fraud'

    profile_uuid = models.UUIDField()
    profile_name = models.CharField(max_length=255)
    config_uuid = models.UUIDField()
    config_name = models.CharField(max_length=255)
    status = models.CharField(max_length=20, choices=StatusEnum.choices)
    transaction_id = models.CharField(max_length=255)
    description = models.TextField(blank=True, null=True)
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        db_table = 'rule_results'
        ordering = ['-created_at']

    def __str__(self):
        return f"{self.transaction_id} - {self.status}"