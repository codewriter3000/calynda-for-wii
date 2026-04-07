# devkitPro base image is Debian Bookworm with devkitPPC pre-installed
FROM devkitpro/devkitppc:latest

ENV DEBIAN_FRONTEND=noninteractive

# Install wii-dev meta-package (libogc, etc.)
RUN dkp-pacman -Syu --noconfirm \
    && dkp-pacman -S --noconfirm --needed wii-dev \
    && dkp-pacman -Scc --noconfirm

# Install build essentials and Dolphin emulator
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential \
       git \
       curl \
       ca-certificates \
       sudo \
       dolphin-emu \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Environment variables for devkitPPC
ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITPPC=${DEVKITPRO}/devkitPPC
ENV PATH=${DEVKITPPC}/bin:${DEVKITPRO}/tools/bin:${PATH}

# Create non-root user for development
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} \
    && echo ${USERNAME} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

USER ${USERNAME}
WORKDIR /workspaces
