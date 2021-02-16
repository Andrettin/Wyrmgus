![pipeline status](
https://github.com/Andrettin/Wyrmgus/workflows/CMake/badge.svg?branch=master)

# GitHub Actions for Wyrmgus

Dirctory contains all necessary file for running CI processes on GitHub.
[Dockerfile](Dockerfile) is used to maintain a GNU/Linux environment to build
and test Wyrmgus. Pipeline fetching the docker image from
https://hub.docker.com, so an updated image should be uploaded. To build an
image run command in directory with [Dockerfile](Dockerfile):
```
$ docker image build -tag 'image_name:tag' --compress --force-rm
```

Pipeline may be ran locally with [act](https://github.com/nektos/act) tool. To
do so run act at root of the repository with a trigger name (requires Docker):
```
$ act -v push`
```

The `-v` key for verbose output.
