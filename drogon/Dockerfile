FROM drogonframework/drogon:latest AS build

WORKDIR /app
COPY . .

# Download dependencies (jwt-cpp and picojson) for JWT auth
RUN rm -rf third_party/jwt-cpp && \
    git clone https://github.com/Thalhammer/jwt-cpp.git third_party/jwt-cpp && \
    mkdir -p third_party/picojson && \
    wget https://raw.githubusercontent.com/kazuho/picojson/master/picojson.h -O third_party/picojson/picojson.h

# Build the Drogon project using CMake
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j$(nproc)

# Use the same base image for runtime to ensure all dependencies are present
FROM drogonframework/drogon:latest

WORKDIR /app
COPY --from=build /app/build/bankara_backend /app/
COPY config.json /app/

EXPOSE 5150

CMD ["./bankara_backend"]
