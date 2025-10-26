import grpc
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel, Field, field_validator, model_validator
from enum import Enum
import datetime
import os

import transaction_pb2
import transaction_pb2_grpc


class TransactionTypeEnum(str, Enum):
    withdrawal = 'withdrawal'
    deposit = 'deposit'
    transfer = 'transfer'
    payment = 'payment'

class DeviceUsedEnum(str, Enum):
    mobile = 'mobile'
    atm = 'atm'
    pos = 'pos'
    web = 'web'

class PaymentChannelEnum(str, Enum):
    upi = 'UPI'
    card = 'card'
    ach = 'ACH'
    wire_transfer = 'wire_transfer'

class TransactionModel(BaseModel):
    transaction_id: str
    timestamp: datetime.datetime
    sender_account: str
    receiver_account: str
    amount: float
    transaction_type: TransactionTypeEnum
    merchant_category: str = Field(..., alias='Merchant_category')
    location: str
    device_used: DeviceUsedEnum
    payment_channel: PaymentChannelEnum
    ip_address: str
    device_hash: str

    @field_validator('amount')
    def amount_must_be_positive(cls, v):
        if v <= 0:
            raise ValueError('amount must be positive')
        return v
    

app = FastAPI()

GRPC_SERVICE_ADDRESS = os.getenv("GRPC_SERVICE_ADDRESS", "localhost:50051")

async def send_transaction_via_grpc(transaction: TransactionModel):
    try:
        async with grpc.aio.insecure_channel(GRPC_SERVICE_ADDRESS) as channel:
            stub = transaction_pb2_grpc.TransactionServiceStub(channel)
            
            grpc_request = transaction_pb2.Transaction(
                transaction_id=transaction.transaction_id,
                sender_account=transaction.sender_account,
                timestamp=transaction.timestamp.isoformat(),
                receiver_account=transaction.receiver_account,
                amount=transaction.amount,
                transaction_type=transaction_pb2.Transaction.TransactionType.Value(transaction.transaction_type.name.upper()),
                merchant_category=transaction.merchant_category,
                location=transaction.location,
                device_used=transaction_pb2.Transaction.DeviceUsed.Value(transaction.device_used.name.upper()),
                payment_channel=transaction_pb2.Transaction.PaymentChannel.Value(transaction.payment_channel.name.upper()),
                ip_address=transaction.ip_address,
                device_hash=transaction.device_hash,
            )
            
            await stub.ProcessTransaction(grpc_request)
            
    except grpc.aio.AioRpcError as e:
        raise HTTPException(
            status_code=503,
            detail=f"Service unavailable: Failed to connect to gRPC service. Details: {e.details()}"
        )

@app.post("/transaction/")
async def process_transaction(transaction: TransactionModel):
    await send_transaction_via_grpc(transaction)
    
    return {"status": "Transaction accepted and forwarded for processing"}