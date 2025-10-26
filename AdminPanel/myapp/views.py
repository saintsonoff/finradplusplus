from django.shortcuts import render
from django.http import HttpResponse
import csv
from .models import RuleResult

def index(requset):
    return HttpResponse("Hello you are here")

def export_rule_results_csv(request):
    """
    Export rule_results table to CSV file
    """
    response = HttpResponse(content_type='text/csv')
    response['Content-Disposition'] = 'attachment; filename="rule_results_export.csv"'
    
    writer = csv.writer(response)
    writer.writerow([
        'ID',
        'Profile UUID',
        'Profile Name',
        'Config UUID',
        'Config Name',
        'Status',
        'Transaction ID',
        'Description',
        'Created At'
    ])
    
    results = RuleResult.objects.all().order_by('-created_at')
    
    for result in results:
        writer.writerow([
            result.id,
            result.profile_uuid,
            result.profile_name,
            result.config_uuid,
            result.config_name,
            result.status,
            result.transaction_id,
            result.description or '',
            result.created_at.strftime('%Y-%m-%d %H:%M:%S')
        ])
    
    return response