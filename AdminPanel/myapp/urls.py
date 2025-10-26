from django.urls import path
from . import views

urlpatterns = [
    path('', views.index, name='index'),
    path('export/rule-results/', views.export_rule_results_csv, name='export_rule_results'),
]
