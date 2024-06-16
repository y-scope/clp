#!/usr/bin/env bash

# NOTE: We use the bash convention where 0 is true and 1 is false

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
root_dir="${script_dir}/../.."

# Parses and prints grep matches in a format relevant to the current environment.
# Arguments:
#   $1: Zero-terminated grep matches to parse and print
parse_and_print_matches () {
    matches="$1"
    while IFS= read -r match; do
        if [ -n "${GITHUB_ACTIONS+x}" ]; then
            # Print a GitHub Actions error annotation
            awk_cmd='{'
            # Print the filename, line number, title, and start the message with the first token
            # of the match
            awk_cmd+='printf "::error file=%s,line=%s,title=Bad docs link::%s", $1, $2, $3'
            # Print remaining tokens of the match
            awk_cmd+='; for (i = 4; i <= NF; i++) {printf ":%s", $i}'
            # End the annotation
            awk_cmd+='; printf "\n"'
            awk_cmd+='}'
            echo "$match" | awk --field-separator ':' "$awk_cmd"
        else
            echo "$match"
        fi
    done <<< "$matches"
}

# Grep for a pattern in all tracked files in the repo (except this script).
# Arguments:
#   $1: The pattern to search for
#   $2: The directory to search in
# Returns:
#   Whether the pattern was found in any file.
grep_tracked_files () {
    pattern="$1"
    dir_to_search="$2"

    found_matches=1

    while IFS= read -d $'\0' -r path; do
        # Skip directories and this script
        if [ "$path" -ef "${BASH_SOURCE[0]}" ] || [ -d "$path" ] ; then
            continue
        fi

        if matches=$(grep --extended-regexp --line-number --with-filename "$pattern" "$path") ; then
            parse_and_print_matches "$matches"
            found_matches=0
        fi
    done < <(git ls-files --cached --exclude-standard -z "$dir_to_search")

    return $found_matches
}

cUsage="Usage: ${BASH_SOURCE[0]}"
if [ "$#" -gt 0 ] ; then
    echo "$cUsage"
    exit
fi

found_violation=1

# Check for https://docs.yscope.com links with ".md" suffixes
if grep_tracked_files "https://docs\\.yscope\\.com/.+\\.md" "$root_dir" ; then
    echo "https://docs.yscope.com links cannot have \".md\" suffixes." >&2
    found_violation=0
fi

# Check for sphixn :link: attributes that have ".md" suffixes
if grep_tracked_files ":link:[[:space:]]*.+\\.md" "${root_dir}/docs"; then
    echo "sphinx :link: attributes cannot have \".md\" suffixes" >&2
    found_violation=0
fi

if [ $found_violation -eq 0 ]; then
    exit 1
fi
