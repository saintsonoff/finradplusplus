
echo "Waiting for database..."

until python -c "import psycopg2; psycopg2.connect(host='${POSTGRES_HOST:-postgres}', port='${POSTGRES_PORT:-5432}', user='${POSTGRES_USER:-postgres}', password='${POSTGRES_PASSWORD:-postgres}', dbname='${POSTGRES_DB:-it_cup_onecode}')" 2>/dev/null; do
  echo "PostgreSQL is unavailable - sleeping"
  sleep 2
done

echo "PostgreSQL is up - executing migrations"

echo "Running database migrations..."
python manage.py makemigrations --noinput
python manage.py migrate --noinput

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
