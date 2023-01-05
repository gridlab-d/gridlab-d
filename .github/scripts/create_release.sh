#!/usr/bin/env bash

curl \
  -X POST \
  -H "${GH_AUTH}" \
  -H "Accept: application/vnd.github.v3+json" \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  "${GLD_GITHUB_URL}/releases" \
  -d '{"tag_name":"'"${GLD_TAG}"'","target_commitish":"master","name":"'"${GLD_TAG}"'","body":"GridLAB-D '"${GLD_TAG}"'","draft":true,"prerelease":false,"generate_release_notes":true}'