FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install system packages
RUN apt-get update && \
    apt-get -y upgrade && \
    apt-get -y install \
    curl \
    git \
    vim \
    zsh \
    ca-certificates \
    cmake \
    swig \
    flex \
    bison \
    libtool \
    zlib1g-dev \
    libx11-dev \
    libboost-dev \
    tcl-dev \
    tk-dev \
    libjpeg-dev \
    python3 \
    python3-pip \
    && apt-get -y autoremove && \
    apt-get clean

# Set Zsh as default shell for root
RUN chsh -s /bin/zsh root

# Install Oh My Zsh for root
RUN curl -Lo /tmp/install.sh https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh && \
    RUNZSH=no CHSH=no KEEP_ZSHRC=yes bash /tmp/install.sh && \
    rm /tmp/install.sh

# Set clean theme in .zshrc
RUN sed -i 's/^ZSH_THEME=.*/ZSH_THEME="maran"/' /root/.zshrc

# Make python command point to python3
RUN update-alternatives --install /usr/bin/python python /usr/bin/python3 1

# Set default working directory to mount point
WORKDIR /mnt/RePlAce

# Launch Zsh by default
CMD ["zsh"]
