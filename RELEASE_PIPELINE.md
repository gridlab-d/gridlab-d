# New Version Release Pipeline
## Version Update
# Version tags
The following files contain version numbers which should be updated for each release:
```
COPYRIGHT
LICENSE
VERSION
cmake/SetVersion.cmake
gldcore/gridlabd.h
gldcore/legal.cpp
```
Note: the name schedule for future releases can be found in `gldcore/legal.cpp`.

## Build Pipeline
### Release Branch 
A release branch tracking the current state of a release should be created from the develop 
branch containing all assets and resolved merges desired for the new version release.

The naming convention for a release branch is `release/MAJOR.MINOR[.PATCH]` (e.g. `release/5.2` or `release/5.2.1`).

### Pipeline verification
Once a `release/*` branch is pushed to the repository it will trigger the build process under 
[Github Actions](https://github.com/gridlab-d/gridlab-d/actions).

The successful compilation of the release branch should be verified before a release is tagged.
When possible, each artifact bundle should be downloaded on an appropriate system and the validation 
suite run and verified for each platform.

### Release Tagging
Once the release branch is complete, a release tag should be created from the release branch following the 
`vMAJOR.MINOR[.PATCH]` notation (e.g. `v5.2` or `v5.2.1`). 

Once a version tag is pushed to the repository it will trigger the build and release process under
[Github Actions](https://github.com/gridlab-d/gridlab-d/actions).

On completion, this pipeline creates a draft release under [Releases](https://github.com/gridlab-d/gridlab-d/releases) 
visible only to project developers which can be reviewed and published for general access.

## Branch resolution
Upon issue of a new release, the release branch should be merged through pull request back to the `develop` branch
with any updates not present there (version numbers, etc.), and to the `master` branch. 

The release branch should be retained as a tracking branch should new patch releases need to be applied 
without requiring a full update to the application. 