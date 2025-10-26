
import grpc
from .models import Profile, RuleConfig, MLRule, ThresholdRule, CompositeRule, PatternRule
from django.conf import settings

from rules import profile_pb2
from rules import profile_pb2_grpc
from rules import rule_config_pb2
from google.protobuf import json_format 

def send_profiles_to_director(profiles_queryset):

    def profile_iterator():
        print("Starting profile iteration for sending...")
        try:
            for profile in profiles_queryset:
                print(f"Processing profile: {profile.name} (UUID: {profile.uuid})")
                grpc_rules = []
                
                for config in profile.rules.all():
                    print(f"  Processing rule config: {config.name} ({config.rule_type})")
                    
                    try:
                        grpc_config = rule_config_pb2.RuleConfig(
                            uuid=str(config.uuid),
                            name=config.name,
                            rule_type=rule_config_pb2.RuleConfig.RuleType.Value(config.rule_type),
                            is_critical=config.is_critical,
                        )

                        rule = config.rule
                        if isinstance(rule, MLRule):
                            grpc_config.ml_rule.model_uuid = rule.model_uuid
                            grpc_config.ml_rule.lower_bound = rule.lower_bound
                            print(f"    Added ML rule: {rule.model_uuid}")
                        elif isinstance(rule, ThresholdRule):
                            print(f"    Converting threshold expression: {rule.expression}")
                            try:
                                json_format.ParseDict(rule.expression, grpc_config.threshold_rule.expression)
                                print(f"    Added threshold rule")
                            except Exception as parse_error:
                                print(f"    ERROR parsing threshold expression: {parse_error}")
                                raise parse_error
                        elif isinstance(rule, CompositeRule):
                            print(f"    Converting composite expression: {rule.expression}")
                            try:
                                json_format.ParseDict(rule.expression, grpc_config.composite_rule.expression)
                                print(f"    Added composite rule")
                            except Exception as parse_error:
                                print(f"    ERROR parsing composite expression: {parse_error}")
                                raise parse_error
                        elif isinstance(rule, PatternRule):
                            grpc_config.pattern_rule.max_delta_time = rule.max_delta_time
                            grpc_config.pattern_rule.max_count = rule.max_count
                            print(f"    Converting pattern expression: {rule.expression}")
                            try:
                                json_format.ParseDict(rule.expression, grpc_config.pattern_rule.expression)
                                print(f"    Added pattern rule")
                            except Exception as parse_error:
                                print(f"    ERROR parsing pattern expression: {parse_error}")
                                raise parse_error
                        else:
                            print(f"    WARNING: Unknown rule type: {type(rule)}")
                        
                        grpc_rules.append(grpc_config)
                        
                    except Exception as e:
                        print(f"    ERROR processing rule config {config.name}: {e}")
                        raise e
                
                grpc_profile = profile_pb2.Profile(
                    uuid=str(profile.uuid),
                    name=profile.name,
                    rules=grpc_rules
                )
                print(f"Sending profile: {profile.name} with {len(grpc_rules)} rules...")
                yield grpc_profile
                
        except Exception as e:
            print(f"ERROR in profile_iterator: {e}")
            import traceback
            traceback.print_exc()
            raise e

    try:
        director_address = getattr(settings, "DIRECTOR_GRPC_ADDRESS", "localhost:50051")
        print(f"Connecting to director at: {director_address}")
        with grpc.insecure_channel(director_address) as channel:
            # Создаем stub для нового сервиса ProfileService
            stub = profile_pb2_grpc.ProfileServiceStub(channel)
            print("gRPC channel created, calling ProcessProfileStream...")
            # Вызываем новый стриминговый метод и передаем ему наш итератор
            # Сервис вернет google.protobuf.empty_pb2.Empty, поэтому мы не присваиваем результат
            response = stub.ProcessProfileStream(profile_iterator(), timeout=30)
            print(f"ProcessProfileStream completed successfully. Response: {response}")
            return True, "Стрим профилей успешно отправлен."
    except grpc.RpcError as e:
        error_msg = f"Ошибка gRPC: {e.code()} - {e.details()}"
        print(f"ERROR: {error_msg}")
        return False, error_msg
    except Exception as e:
        error_msg = f"Произошла непредвиденная ошибка: {str(e)}"
        print(f"ERROR: {error_msg}")
        import traceback
        traceback.print_exc()
        return False, error_msg