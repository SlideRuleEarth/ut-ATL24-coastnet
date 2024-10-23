#!/usr/bin/bash

# Set bash strict mode
set -euo pipefail
IFS=$'\n\t'

# Create the prompt
prompt_string="Are you sure? "

# Check command line
if [ $# -ne 0 ]; then
    prompt_string="$1 $prompt_string"
fi

# Prompt for 'Y' key
read -p ${prompt_string} -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
    # User did not press 'Y'
    echo "Aborted"
    exit -1
fi

# User pressed 'Y'
exit 0
