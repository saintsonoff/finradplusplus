FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

WORKDIR /director_service

COPY ./director_service/ .

COPY ./shared_data_model /shared_data_model

RUN . ./.env && \
    cmake -S ./src -B ./build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build ./build

CMD ["./build/bin/director_service", "--config", "./configs/static_config.yaml"]