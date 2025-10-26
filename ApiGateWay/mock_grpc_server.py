import asyncio
import logging
import grpc

# Импортируем сгенерированные классы
import transaction_pb2
import transaction_pb2_grpc

# Импортируем Empty для ответа, как указано в .proto
from google.protobuf import empty_pb2

# --- 1. Реализация сервиса ---
# Создаем класс, который наследуется от сгенерированного ...Servicer
# и реализует его методы.
class TransactionServiceServicer(transaction_pb2_grpc.TransactionServiceServicer):
    """
    Реализует gRPC сервис TransactionService.
    """

    async def ProcessTransaction(
        self,
        request: transaction_pb2.Transaction,
        context: grpc.aio.ServicerContext
    ) -> empty_pb2.Empty:
        """
        Принимает и "обрабатывает" одну транзакцию.
        В нашем мок-сервере мы просто выводим полученные данные в консоль.
        """
        logging.info("="*30)
        logging.info("[gRPC Server] Received new transaction:")
        
        # Выводим все поля полученного запроса для проверки
        logging.info(f"  Transaction ID: {request.transaction_id}")
        logging.info(f"  Timestamp: {request.timestamp}")
        logging.info(f"  Sender: {request.sender_account}")
        logging.info(f"  Receiver: {request.receiver_account}")
        logging.info(f"  Amount: {request.amount:.2f}")
        
        # Для Enum полей можно получить и числовое значение, и строковое имя
        transaction_type_name = transaction_pb2.Transaction.TransactionType.Name(request.transaction_type)
        logging.info(f"  Type: {transaction_type_name} ({request.transaction_type})")
        
        logging.info(f"  Merchant Category: {request.merchant_category}")
        
        device_used_name = transaction_pb2.Transaction.DeviceUsed.Name(request.device_used)
        logging.info(f"  Device Used: {device_used_name} ({request.device_used})")
        
        logging.info(f"  IP Address: {request.ip_address}")
        logging.info("="*30)

        # Имитируем небольшую задержку на обработку
        await asyncio.sleep(0.5)

        logging.info("[gRPC Server] Transaction processed successfully.")

        # Возвращаем пустой ответ, как того требует proto-схема
        return empty_pb2.Empty()


# --- 2. Функция для запуска сервера ---
async def serve() -> None:
    """
    Основная функция для конфигурации и запуска gRPC сервера.
    """
    server = grpc.aio.server()
    
    # "Привязываем" нашу реализацию сервиса к серверу
    transaction_pb2_grpc.add_TransactionServiceServicer_to_server(
        TransactionServiceServicer(), server
    )
    
    # Указываем порт, который будет слушать сервер
    # '[::]:50051' означает, что он будет слушать на всех доступных
    # сетевых интерфейсах (включая localhost)
    listen_addr = '[::]:50051'
    server.add_insecure_port(listen_addr)
    
    logging.info(f"Starting gRPC server on {listen_addr}")
    
    await server.start()
    
    # Сервер будет работать до тех пор, пока мы его не остановим (Ctrl+C)
    await server.wait_for_termination()


# --- 3. Точка входа в программу ---
if __name__ == '__main__':
    # Настраиваем базовое логирование, чтобы видеть сообщения
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    try:
        asyncio.run(serve())
    except KeyboardInterrupt:
        logging.info("gRPC server stopped.")