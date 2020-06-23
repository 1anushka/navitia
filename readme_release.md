# HOW TO Release navitia

## Versionning

Our versionning is based on [Semantic Versionning](nhttps://semver.org/)
* the **major** version is bumped in case of an API/interface change or **when a binarisation is needed**.
* the **minor** version is bumped when functionnalities are backward compatible.
* the **patch** version is bumped on a bug fix.

## "Regular" release

This lets the script decide if it's major or minor release.
The decision is based on the data_version located in source/type/data.cpp
See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how data_version is managed.

First have a look on github's repo at PRs about to be released https://github.com/CanalTP/navitia/pulls?q=is%3Apr+is%3Aclosed+sort%3Aupdated-desc
* Check that `not_in_changelog` and `hotfix` labels are correct and none is missing on PRs that are gonna be released
* Check that titles are correct (clear, with the component impacted)

Then the script should take over:
```sh
cd <path/to/repo/navitia>
# to be sure to trigger the correct version of the release script
git fetch <canaltp_distant_repo_name> && git rebase <canaltp_distant_repo_name>/dev dev
```
At this point, you may build and run tests to check that everything is OK. If you're confident, proceed with:
```sh
./release_navitia.sh <canaltp_distant_repo_name>
```
Note: this script uses "vim", make sure it's installed on your machine.

Then follow the instructions given by the script, and also:
* pay attention to the changelog, remove useless PR (small doc) and check that every important PR is there
* don't forget to make `git submodule update --recursive` when necessary
* check that `release` branch COMPILES and TESTS (unit, docker and tyr) before pushing it!

Nota: `major` and `minor` invocations are possible but deprecated.

## For hotfix

Note: It is preferable but not mandatory to merge the hotfix PR before.
```sh
./release_navitia.sh <canaltp_distant_repo_name>
```
A new branch has been created <release_x.yy.z> and the changelog is opened.
Then the process is less automated (but still, instructions are given):
* Populate the changelog :
	Add the hotfix PR name and link to github (as for the automated changelog in the release process)
* Cherry-pick the commits you want to release:
	```
	git cherry-pick <commit_id> # Each commit_id of the hotfix PR
	```
* Merge the content of the new release branch with the hotfix commits to the 'release' branch:
	```
	git checkout release
	git merge --no-ff <release_x.yy.z>
	```
* Tag the new release:
	```
	git tag -a vx.yy.z
	```
    _Minor_: You will have to populate the tag with correct blank lines if you want a nice github changelog:
    ```
    Version 2.57.0

        * Kraken: Add ptref shortcut between physical_mode and jpps  <https://github.com/CanalTP/navitia/pull/2417>
    ```

## And Finally

* Merge the 'release' branch to the 'dev' branch:
	```
	git checkout dev
	git merge --ff release
	```
* Push the release and dev branches to the repo
	```
	git push <canaltp_distant_repo_name> release dev --tags
	```

# Troubleshooting
If you run into github's daily limitation, you can easily provide your login/token into the script.
Search for "rate limit exceeded" in script.
