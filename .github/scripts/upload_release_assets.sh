#!/usr/bin/env bash

# Validate token.
curl -o /dev/null -H "Authorization: token ${GITHUB_TOKEN}" "${GLD_GITHUB_URL}" || {
  echo "Error: Invalid repo, token or network issue!"
  exit 1
}

# Generate new draft release
RELEASE_INFO=$(curl \
  -X POST \
  -H "Accept: application/vnd.github.v3+json" \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  "${GLD_GITHUB_URL}/releases" \
  -d '{"tag_name":"'"${GLD_TAG}"'","target_commitish":"master","name":"'"${GLD_TAG}"'","body":"GridLAB-D '"${GLD_TAG}"'","draft":true,"prerelease":false,"generate_release_notes":true}')
cat "$RELEASE_INFO"

# Read asset tags.
UPLOAD_URL=$(echo "${RELEASE_INFO}" | jq -r '.upload_url' | cut -d'{' -f1)
echo "${UPLOAD_URL}"
release_id=$(echo "${RELEASE_INFO}" | jq '.id')
echo "${release_id}"
[ "$release_id" ] || {
  echo "Error: Failed to get release id for tag: ${GLD_TAG}"
  echo "${RELEASE_INFO}" | awk 'length($0)<100' >&2
  exit 1
}
dir=$(pwd)
for FILE in "${dir}"/* "${dir}"/**/*; do
  # Upload asset
  echo "Uploading ${FILE}"

  curl --data-binary @"${FILE}" \
    -H "Authorization: token ${GITHUB_TOKEN}" \
    -H "Content-Type: application/octet-stream" \
    "${UPLOAD_URL}?name=$(basename "${FILE}")" | cat
done
