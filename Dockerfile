FROM debian:stretch
RUN apt update && apt install -y build-essential libbsd0 libbsd-dev
RUN mkdir -p /app/build
COPY . /app
WORKDIR /app
CMD ["./scripts/make-package.sh"]
