FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    bash \
    git \
    libc6-dev \
    libncurses5-dev \
    libncursesw5-dev \
    libbsd-dev \
    pty-process \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/programming-fundamentals-class/project-2024a-ppaa23.git /app

WORKDIR /app

RUN make

CMD ["make", "run"]
