FROM fedora:36 AS builder

LABEL maintainer="Ed Beroset <beroset@ieee.org>"

WORKDIR /tmp/
ENV TZ=America/New_York
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN dnf install -y gcc-c++ git cmake gnuplot sqlite-devel gsl-devel
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git
WORKDIR /tmp/ns-3-dev/
RUN git checkout a736c7a09d34fda3260148ad247794325b4c9137
RUN ./ns3 configure --build-profile=debug --enable-examples --enable-tests
RUN ./ns3 build
ENTRYPOINT ["./ns3","run","--cwd=scratch/epri"]
CMD ["scratch-simulator.cc"]

FROM builder
RUN dnf install -y doxygen dia python3-sphinx texlive-collection-fontsrecommended \
    texlive-collection-latexextra
RUN ./ns3 docs all
