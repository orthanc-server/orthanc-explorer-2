I know, this is old style manual tests !  Anyone willing to automate this is pretty welcome !


Modifications
=============

Prerequisites:

- make sure there are no patients whose `PatientID` starts with `TEST`
- upload the test study from `tests/stimuli/TEST_1`

Edit study tags to create a modified copy:
- on Study `Test CT`, `Modify Study tags` and change:
  - `PatientBirthDate = 19500630`
  - `PatientID = TEST_2`
  - `PatientName = Test2`
  - `AccessionNumber = 2345`
  - `StudyDate = 20150101`
  - `StudyID = 2`
  - Insert `InstitutionName = MY`
  - Remove `StudyDescription`
  - select "Create a modified copy of the original study"
  - Apply
  - Check: You should now have 2 studies for `PatientID = TEST*`
  - Check the new study has changed its tags

Edit study tags 'in place':
- on the new Study, `Modify Study tags`, first copy the `StudyInstanceUID` and change:
  - `StudyDate = 20200101`
  - select "Modify the original study. (keeping the original DICOM UIDs)"
  - Apply
  - Check:
    - `StudyDate == 20200101`
    - `StudyInstanceUID` has not changed
    ----- > TODO: right now: needs reload


Edit study tags, can not be used to edit a patient who already has other studies
- on the new Study, `Modify Study tags` and change:
  - `PatientID = TEST_1`
  - don't change other Patient tags
  - try all 3 modification mode options
  - Apply
  - Check: you should get an error telling you to use `Change Patient`

Edit study tags, can be used to create a new patient
- on the new Study, `Modify Study tags` and change:
  - `PatientID = TEST_3`
  - `PatientName = Test3`
  - don't change other Patient tags
  - select "Create a modified copy of the original study"
  - Apply
  - Check: you should now have 3 studies and 3 patients

Attach study to a non existing patient
- on any Study, `Change patient` and set:
  - `PatientID = NO_SUCH_PATIENT`
  - Apply
  - Check: you should get en error telling you to use `Modify Study tags` instead

Attach study to an existing patient, keep DICOM UIDs
- on Study from `PatientID = TEST_3`, `Change patient` and set:
  - `PatientID = TEST_1`
  - select "Modify the original study. (keeping the original DICOM UIDs)"
  - Apply
  - Check: 
    - Patient `TEST_1` shall now have 2 studies
    - the Patient tags shall be identical in both studies

Edit patient birth date in multiple studies
- on a study from `PatientID = TEST_1`, 'Modify Patient tags' and set:
  - `PatientBirthDate = 19100101`
  - select "Modify the original study. (keeping the original DICOM UIDs)"
  - Apply
  - Check:
    - `PatientBirthDate` has changed in all studies, 
    ----- > TODO: right now: needs reload

Edit patient ID in multiple studies
- on a study from `PatientID = TEST_1`, 'Modify Patient tags' and set:
  - `PatientID = TEST_4`
  - select "Modify the original study. (keeping the original DICOM UIDs)"
  - Apply
  - Check:
    - `PatientID` has changed in all studies, 
    ----- > TODO: right now: needs reload

Edit patient ID in multiple studies
- on a study from `PatientID = TEST_4`, 'Modify Patient tags' and set:
  - `PatientID = TEST_1`
  - select "Create a modified copy of the original study"
  - Apply
  - Check:
    - there are duplicate studies for Patient `TEST_1` and `TEST_4`
    ----- > TODO: right now: needs reload


