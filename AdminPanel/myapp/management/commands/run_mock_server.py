import grpc
from concurrent import futures
import time
from django.core.management.base import BaseCommand

from rules import profile_pb2_grpc
from rules import rule_config_pb2
from google.protobuf import empty_pb2

class ProfileService(profile_pb2_grpc.ProfileServiceServicer):
    def ProcessProfileStream(self, request_iterator, context):
        """
        Этот метод будет вызван, когда Django-клиент отправит стрим профилей.
        'request_iterator' - это итератор объектов Profile.
        """
        print("✅ === Получен стрим профилей от Django Admin! === ✅")
        
        try:
            profiles_received = 0
            
            for profile in request_iterator:
                profiles_received += 1
                print(f"\n--- Профиль ---")
                print(f"  UUID: {profile.uuid}")
                print(f"  Name: {profile.name}")
                print(f"  Количество правил: {len(profile.rules)}")
                
                for rule_config in profile.rules:
                    print(f"    - Конфиг '{rule_config.name}' (UUID: {rule_config.uuid})")
                    print(f"      Тип правила: {rule_config_pb2.RuleConfig.RuleType.Name(rule_config.rule_type)}")
                    print(f"      Критичность: {rule_config.is_critical}")

                    active_rule_field = rule_config.WhichOneof('rule')

                    if active_rule_field:
                        rule_object = getattr(rule_config, active_rule_field)
                        
                        from google.protobuf import json_format
                        rule_json = json_format.MessageToDict(rule_object)
                        print(f"      Содержимое правила (JSON): {rule_json}")
                    else:
                        print("      Содержимое правила: (не задано)")
            
            print(f"\n✅ === Обработано {profiles_received} профилей! === ✅")
            
            return empty_pb2.Empty()
        except Exception as e:
            print(f"❌ Ошибка при обработке запроса: {e}")
            context.set_code(grpc.StatusCode.INTERNAL)
            context.set_details(str(e))
            return empty_pb2.Empty()

class Command(BaseCommand):
    help = 'Запускает gRPC мок-сервер для сервиса Director'

    def handle(self, *args, **options):
        server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
        profile_pb2_grpc.add_ProfileServiceServicer_to_server(ProfileService(), server)
        
        port = "50051"
        server.add_insecure_port(f"[::]:{port}")
        
        server.start()
        self.stdout.write(self.style.SUCCESS(f"🚀 Мок-сервер Director запущен на порту {port}..."))
        
        try:
            server.wait_for_termination()
        except KeyboardInterrupt:
            self.stdout.write(self.style.WARNING("🛑 Сервер останавливается."))
            server.stop(0)