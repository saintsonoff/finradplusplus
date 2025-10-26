import re
from collections import namedtuple


Token = namedtuple('Token', ['type', 'value'])

class Lexer:
    """
    Разбивает входную строку на последовательность токенов.
    """
    token_specification = [
        ('NUMBER',       r'\d+(\.\d*)?'),
        ('STRING',       r"'[^']*'|\"[^\"]*\""),  
        ('BOOL',         r'TRUE|FALSE'),
        ('AGG_FUNCTION', r'COUNT_DISTINCT|COUNT|SUM|AVG|MIN|MAX'),
        ('FIELD',        r'TRANSACTION_ID|SENDER_ACCOUNT|TIMESTAMP|RECEIVER_ACCOUNT|AMOUNT|TRANSACTION_TYPE|MERCHANT_CATEGORY|LOCATION|DEVICE_USED|PAYMENT_CHANNEL|IP_ADDRESS|DEVICE_HASH|TIME'),
        ('LOGIC_OP',     r'AND|OR|NOT'),
        ('COMP_OP',      r'==|!=|>=|<=|>|<|='),  
        ('LPAREN',       r'\('),
        ('RPAREN',       r'\)'),
        ('WILDCARD',     r'\*'),  # Добавляем токен для *, должен быть после скобок
        ('IDENTIFIER',   r'[A-Za-z_][A-Za-z0-9_]*'), 
        ('SKIP',         r'[ \t\n\r]+'),  
        ('MISMATCH',     r'.'),
    ]
    tok_regex = '|'.join('(?P<%s>%s)' % pair for pair in token_specification)

    def __init__(self, text: str):
        self.text = text.upper()

    def tokenize(self):
        for mo in re.finditer(self.tok_regex, self.text):
            kind = mo.lastgroup
            value = mo.group()
            if kind == 'SKIP':         
                continue
            if kind == 'MISMATCH':
                raise ValueError(f"Unknown character in expression: '{value}'. Check spelling of fields, operators and values.")
            yield Token(kind, value)


class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def peek(self):
        if self.pos < len(self.tokens):
            return self.tokens[self.pos]
        return None

    def consume(self, expected_type=None):
        token = self.peek()
        if token is None:
            raise ValueError("Unexpected end of expression.")
        if expected_type and token.type != expected_type:
            raise ValueError(f"Expected token type {expected_type}, but got {token.type} ('{token.value}')")
        self.pos += 1
        return token

    COMP_OP_MAP = {
        "==": "EQUAL", "!=": "NOT_EQUAL", ">": "GREATER_THAN", ">=": "GREATER_THAN_OR_EQUAL",
        "<": "LESS_THAN", "<=": "LESS_THAN_OR_EQUAL", "=": "EQUAL"
    }
    LOGIC_OP_MAP = {"AND": "AND", "OR": "OR", "NOT": "NOT"}
    AGG_FUNC_MAP = {
        "COUNT": "COUNT", "SUM": "SUM", "AVG": "AVG", "MIN": "MIN", "MAX": "MAX", 
        "COUNT_DISTINCT": "COUNT_DISTINCT"
    }

    def _build_logical(self, left, op_token, right):
        op_str = self.LOGIC_OP_MAP[op_token.value]
        if isinstance(left, dict) and left.get("logical", {}).get("operator") == op_str:
            left["logical"]["operands"].append(right)
            return left
        return {"logical": {"operator": op_str, "operands": [left, right]}}
    
    def parse_atom(self):
        token = self.peek()
        if token is None:
            raise ValueError("Unexpected end of expression, expected operand.")
            
        if token.type == 'NUMBER':
            val = float(self.consume().value)
            if val.is_integer():
                int_val = int(val)
                if -2147483648 <= int_val <= 2147483647:
                    return {"literal": {"int_value": int_val}}
                else:
                    return {"literal": {"int64_value": int_val}}
            else:
                return {"literal": {"float_value": val}}
        elif token.type == 'STRING':
            return {"literal": {"string_value": self.consume().value[1:-1]}}
        elif token.type == 'BOOL':
            return {"literal": {"bool_value": self.consume().value == 'TRUE'}}
        elif token.type == 'FIELD':
            return {"field": {"field": self.consume().value}}
        elif token.type == 'IDENTIFIER':
            return {"literal": {"string_value": self.consume().value.lower()}}
        elif token.type == 'AGG_FUNCTION':
            func_token = self.consume()
            self.consume('LPAREN')
            
            # Проверяем, является ли следующий токен *
            token = self.peek()
            if token and token.type == 'WILDCARD':
                # Потребляем * и создаем специальный узел для COUNT(*)
                self.consume()
                operand_node = {"wildcard": {}}  # Специальный узел для *
            else:
                # Обычный случай: получаем поле или другое выражение
                operand_node = self.parse_atom()
                if "field" not in operand_node and "wildcard" not in operand_node:
                    raise ValueError("Aggregate function argument must be a field or '*' (e.g., AMOUNT or *).")
            
            self.consume('RPAREN')
            return {"aggregate": {"function": self.AGG_FUNC_MAP[func_token.value], "operand": operand_node}}
        elif token.type == 'LPAREN':
            self.consume('LPAREN')
            node = self.parse_logical_expression()
            self.consume('RPAREN')
            return node
        else:
            raise ValueError(f"Unexpected token: '{token.value}'. Expected number, string, field or aggregate function.")

    def parse_comparison(self):
        left_node = self.parse_atom()
        token = self.peek()
        if token and token.type == 'COMP_OP':
            op_token = self.consume()
            right_node = self.parse_atom()
            return {"comparison": {"operator": self.COMP_OP_MAP[op_token.value], "left": left_node, "right": right_node}}
        return left_node

    def parse_logical_factor(self):
        token = self.peek()
        if token and token.type == 'LOGIC_OP' and token.value == 'NOT':
            self.consume()
            operand_node = self.parse_logical_factor()
            return {"logical": {"operator": "NOT", "operands": [operand_node]}}
        return self.parse_comparison()

    def parse_logical_term(self):
        node = self.parse_logical_factor()
        while True:
            token = self.peek()
            if token and token.type == 'LOGIC_OP' and token.value == 'AND':
                op_token = self.consume()
                right_node = self.parse_logical_factor()
                node = self._build_logical(node, op_token, right_node)
            else:
                break
        return node

    def parse_logical_expression(self):
        node = self.parse_logical_term()
        while True:
            token = self.peek()
            if token and token.type == 'LOGIC_OP' and token.value == 'OR':
                op_token = self.consume()
                right_node = self.parse_logical_term()
                node = self._build_logical(node, op_token, right_node)
            else:
                break
        return node
    
    def parse_entrypoint(self, rule_type: str):
        if rule_type == "THRESHOLD":
            # THRESHOLD: простое сравнение полей
            # Пример: AMOUNT > 1000
            result_node = self.parse_comparison()
        elif rule_type == "PATTERN":
            # PATTERN: сравнение с агрегатной функцией
            # Пример: COUNT(AMOUNT) > 5
            # Структура должна быть: aggregate COMP_OP literal
            left_node = self.parse_atom()
            
            # Проверяем, что левая часть - это агрегатная функция
            if "aggregate" not in left_node:
                raise ValueError(
                    "PATTERN rule left side must be an aggregate function (COUNT, SUM, AVG, etc.).\n"
                    "Пример: COUNT(AMOUNT) > 5 или SUM(AMOUNT) >= 1000"
                )
            
            token = self.peek()
            if token and token.type == 'COMP_OP':
                op_token = self.consume()
                right_node = self.parse_atom()
                
                # Проверяем, что правая часть - это литерал (число или строка)
                if "literal" not in right_node:
                    raise ValueError(
                        "PATTERN rule right side must be a literal value (number or string), not a field.\n"
                        "Пример: COUNT(AMOUNT) > 5 (не COUNT(AMOUNT) > OTHER_FIELD)"
                    )
                
                result_node = {"comparison": {"operator": self.COMP_OP_MAP[op_token.value], "left": left_node, "right": right_node}}
            else:
                raise ValueError("PATTERN rule must contain a comparison operator (>, <, >=, <=, =, !=).")
        elif rule_type == "COMPOSITE":
            result_node = self.parse_logical_expression()
        else:
            raise ValueError(f"Unknown rule type for parser: {rule_type}")
        
        remaining_token = self.peek()
        if remaining_token is not None:
            raise ValueError(f"Failed to process expression completely. Unprocessed token remains: '{remaining_token.value}'")
        
        return result_node

class RuleParser:
    # Список допустимых полей
    VALID_FIELDS = {
        'TRANSACTION_ID', 'SENDER_ACCOUNT', 'TIMESTAMP', 'RECEIVER_ACCOUNT',
        'AMOUNT', 'TRANSACTION_TYPE', 'MERCHANT_CATEGORY', 'LOCATION',
        'DEVICE_USED', 'PAYMENT_CHANNEL', 'IP_ADDRESS', 'DEVICE_HASH', 'TIME'
    }
    
    def parse(self, text: str, rule_type: str) -> dict:
        if not text:
            raise ValueError("Expression cannot be empty.")
        
        text = text.strip()
        if not text:
            raise ValueError("Expression cannot be empty.")
            
        lexer = Lexer(text)
        tokens = list(lexer.tokenize())
        
        if not tokens:
            raise ValueError("Expression must contain at least one valid token.")
        
        # Валидация названий полей
        field_tokens = [t for t in tokens if t.type == 'FIELD']
        if field_tokens:
            invalid_fields = []
            for token in field_tokens:
                if token.value not in self.VALID_FIELDS:
                    invalid_fields.append(token.value)
            
            if invalid_fields:
                valid_fields_str = ', '.join(sorted(self.VALID_FIELDS))
                raise ValueError(
                    f"Неизвестные поля: {', '.join(invalid_fields)}.\n"
                    f"Допустимые поля: {valid_fields_str}"
                )
        
        # Validate for THRESHOLD rules
        if rule_type == "THRESHOLD":
            if any(t.type == 'LOGIC_OP' for t in tokens):
                raise ValueError(
                    "THRESHOLD rules do not support logical operators (AND, OR, NOT).\n"
                    "Use only comparison operators: >, <, >=, <=, =, !=\n"
                    "Пример: AMOUNT > 1000"
                )
            if any(t.type == 'AGG_FUNCTION' for t in tokens):
                raise ValueError(
                    "THRESHOLD rules do not support aggregate functions (COUNT, SUM, AVG, etc.).\n"
                    "Use only field comparisons.\n"
                    "Пример: AMOUNT > 1000"
                )
            # THRESHOLD должен содержать как минимум одно сравнение
            if not any(t.type == 'COMP_OP' for t in tokens):
                raise ValueError(
                    "THRESHOLD rule must contain at least one comparison operator (>, <, >=, <=, =, !=).\n"
                    "Пример: AMOUNT > 1000"
                )
        
        # Validate for PATTERN rules
        if rule_type == "PATTERN":
            if any(t.type == 'LOGIC_OP' for t in tokens):
                raise ValueError(
                    "PATTERN rules do not support logical operators (AND, OR, NOT).\n"
                    "Use aggregate functions with comparison operators.\n"
                    "Пример: COUNT(AMOUNT) > 5 или SUM(AMOUNT) >= 1000"
                )
            # PATTERN должен содержать хотя бы одну агрегатную функцию
            if not any(t.type == 'AGG_FUNCTION' for t in tokens):
                raise ValueError(
                    "PATTERN rule must contain at least one aggregate function (COUNT, SUM, AVG, MIN, MAX, COUNT_DISTINCT).\n"
                    "Пример: COUNT(AMOUNT) > 5 или SUM(AMOUNT) >= 1000"
                )
            # PATTERN должен содержать хотя бы один оператор сравнения
            if not any(t.type == 'COMP_OP' for t in tokens):
                raise ValueError(
                    "PATTERN rule must contain at least one comparison operator (>, <, >=, <=, =, !=).\n"
                    "Пример: COUNT(AMOUNT) > 5 или SUM(AMOUNT) >= 1000"
                )
        
        # Validate for COMPOSITE rules
        if rule_type == "COMPOSITE":
            if not any(t.type == 'LOGIC_OP' for t in tokens):
                raise ValueError(
                    "COMPOSITE rule must contain at least one logical operator (AND, OR, NOT).\n"
                    "Пример: AMOUNT > 100 AND TIME = 'night'"
                )
            if any(t.type == 'AGG_FUNCTION' for t in tokens):
                raise ValueError(
                    "COMPOSITE rules do not support aggregate functions (COUNT, SUM, etc.).\n"
                    "Use logical operators with field comparisons.\n"
                    "Пример: AMOUNT > 100 AND TIME = 'night'"
                )

        parser = Parser(tokens)
        try:
            result = parser.parse_entrypoint(rule_type)
            return result
        except ValueError as e:
            # Улучшаем сообщения об ошибках
            error_msg = str(e)
            if "Unexpected token" in error_msg:
                raise ValueError(
                    f"Синтаксическая ошибка в выражении: {error_msg}\n"
                    f"Проверьте правильность написания полей, операторов и значений."
                )
            elif "Unexpected end of expression" in error_msg:
                raise ValueError(
                    f"Выражение не завершено. {error_msg}\n"
                    f"Возможно, пропущен оператор или значение."
                )
            elif "Failed to process expression completely" in error_msg:
                raise ValueError(
                    f"Лишние символы в конце выражения: {error_msg}\n"
                    f"Удалите лишние токены."
                )
            elif "Expected" in error_msg and "but got" in error_msg:
                raise ValueError(f"Синтаксическая ошибка: {error_msg}")
            else:
                raise ValueError(error_msg)