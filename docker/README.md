# Setup
- Clone the repo
- cd into the repo
    - cd cfsat
- docker build -t cfs-workshop -f docker/Dockerfile .
- docker run -ti --name cfs-workshop-container cfs-workshop /bin/bash
- docker exec -ti cfs-workshop-container /bin/bash