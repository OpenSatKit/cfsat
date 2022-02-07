# Setup
- Clone the repo
- cd into the repo
    - cd cfsat
- docker build -t cfs-workshop -f docker/Dockerfile .
- docker run -ti --privileged --net=host --rm --name cfs-workshop-container cfs-workshop build/exe/cpu1/core-cpu1

