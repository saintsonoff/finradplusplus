FROM python:3.11-slim

WORKDIR /app

COPY ApiGateWay/ .

RUN pip install --no-cache-dir fastapi uvicorn pydantic grpcio grpcio-tools protobuf

EXPOSE 8080

CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8080"]
