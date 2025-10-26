FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV DJANGO_SETTINGS_MODULE=AdminPanel.settings

WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        gcc \
        python3-dev \
    && rm -rf /var/lib/apt/lists/*

COPY ./AdminPanel/requirements.txt /app/
RUN pip install --no-cache-dir -r requirements.txt

COPY ./AdminPanel/ /app/

RUN chmod +x /app/entrypoint.sh

RUN mkdir -p /app/staticfiles

EXPOSE 8000

HEALTHCHECK --interval=30s --timeout=30s --start-period=5s --retries=3 \
    CMD python -c "import socket; s=socket.socket(); s.settimeout(2); result = s.connect_ex(('127.0.0.1', 8000)); s.close(); exit(0 if result == 0 else 1)" || exit 1

ENTRYPOINT ["/app/entrypoint.sh"]

CMD ["gunicorn", "--bind", "0.0.0.0:8000", "--workers", "3", "AdminPanel.wsgi:application"]
