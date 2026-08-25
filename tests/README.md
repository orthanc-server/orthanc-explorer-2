Sample commands to run tests from the `tests/` folder
-----------------------------------------------------

Dev mode (`npm run dev` is running but no other Orthanc).

```
pytest test_study_list_conf2.* -v -s --orthanc-url=http://localhost:3000 --orthanc-mode=orthanc-native --orthanc-exe=/home/alain/o/builds/orthanc/Orthanc --no-header

```

With a local Orthanc already running.

```
pytest test_study_list_conf2.* -v -s --orthanc-url=http://localhost:3000 --orthanc-mode=existing-orthanc --no-header

```

With a native Orthanc.

```
pytest . -v -s --orthanc-mode=orthanc-native --orthanc-exe=/home/alain/o/builds/orthanc/Orthanc --no-header

```
