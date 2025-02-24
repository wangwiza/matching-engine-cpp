# Assignment 1

## Docker

Build docker image
```
docker build -t CS3211/build -f Dockerfile.build .
```

Run image as build container
```
docker run --name build -v "./:/home/ubuntu/workspace" -it CS3211/build
```

Remove container
```
docker rm -f build
```
