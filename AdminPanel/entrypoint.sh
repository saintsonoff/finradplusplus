#!/bin/bash

echo "Waiting for database..."

until python -c "import psycopg2; psycopg2.connect(host='${POSTGRES_HOST:-postgres}', port='${POSTGRES_PORT:-5432}', user='${POSTGRES_USER:-postgres}', password='${POSTGRES_PASSWORD:-postgres}', dbname='${POSTGRES_DB:-it_cup_onecode}')" 2>/dev/null; do
  echo "PostgreSQL is unavailable - sleeping"
  sleep 2
done

echo "PostgreSQL is up - executing migrations"

echo "Running database migrations..."
# Try to apply all migrations
python manage.py migrate --noinput 2>&1 | tee /tmp/migrate.log || true

# If rule_results table already exists, fake the migration
if grep -q "rule_results.*already exists" /tmp/migrate.log; then
    echo "Table rule_results already exists, faking migration..."
    python manage.py migrate myapp 0002_ruleresult --fake || true
fi

# Apply any remaining migrations
python manage.py migrate --noinput || echo "Some migrations may have been skipped"

echo "Collecting static files..."
python manage.py collectstatic --noinput

echo "Creating superuser..."
python manage.py shell -c "
from django.contrib.auth import get_user_model
User = get_user_model()
if not User.objects.filter(username='admin').exists():
    User.objects.create_superuser('admin', 'admin@example.com', 'admin123')
    print('Superuser created: admin/admin123')
else:
    print('Superuser already exists')
"

echo "Starting Django application..."
exec "$@"
