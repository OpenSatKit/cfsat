# NOTE: This is only used to containerize the build and FSW portion, and not the python ground system

# Setup
- Clone the repo
- cd into the repo
    - cd cfsat
- docker build -t cfs-workshop -f docker/Dockerfile .
- docker run -ti --privileged --net=host --rm --name cfs-workshop-container cfs-workshop build/exe/cpu1/core-cpu1

