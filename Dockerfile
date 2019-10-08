FROM alpine as build
WORKDIR /src
COPY ./src ./
RUN apk update && apk add g++ make
RUN make -s
RUN mkdir -p /opt/pickle
RUN mv /src/pickle /opt/pickle

FROM alpine as final
WORKDIR /app
COPY --from=build /opt/pickle .
CMD ./pickle