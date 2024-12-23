FROM manjarolinux/base
LABEL authors="ab-flies"

RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm procps-ng
RUN pacman -S --noconfirm neovim
RUN pacman -S --noconfirm git
RUN pacman -S --noconfirm make cmake gcc
RUN pacman -S --noconfirm glib2
RUN pacman -S --noconfirm librdkafka
RUN pacman -S --noconfirm core/ncurses
RUN pacman -S --noconfirm boost boost-libs

RUN git clone https://github.com/laserpants/dotenv-cpp
RUN cd dotenv-cpp && rm -rf build && mkdir build && cd build && cmake .. && make && sudo make install
RUN git clone https://github.com/mfontanini/cppkafka
RUN cd cppkafka && mkdir build && cd build && cmake .. && make && sudo make install
RUN git clone https://github.com/Neargye/magic_enum
RUN cd magic_enum && mkdir build && cd build && cmake .. && make && sudo make install

RUN pacman -S --noconfirm nodejs npm core/pkgconf
RUN pacman -S --noconfirm fmt
RUN pacman -S --noconfirm extra/mariadb-clients

RUN mkdir /app
WORKDIR /app

COPY ./src ./src
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./res ./res
COPY .example-env ./.env
COPY ./front/*.* ./front/
COPY ./front/addons ./front/addons
COPY ./front/server ./front/server
COPY ./front/src ./front/src

RUN mkdir build && cd build && cmake .. && make && cd ..
RUN cd front && npm install && vite build && node-gyp configure && node-gyp build

RUN npm install -g vite typescript