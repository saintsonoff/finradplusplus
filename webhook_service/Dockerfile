FROM python:3.11-slim

WORKDIR /app

COPY webhook_server.py .

EXPOSE 8888

CMD ["python", "-u", "webhook_server.py"]
