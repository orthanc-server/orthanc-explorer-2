I know, this is old style manual tests !  Anyone willing to automate this is pretty welcome !


Modifications
=============

Prerequisites:

- make sure there are no patients whose `PatientID` starts with `TEST`
- upload the test study from `tests/stimuli/TEST_1`

Study modification
------------------


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
- on the new Study (the one without `StudyDescription`), first copy-paste the `StudyInstanceUID` somewhere
  - click `Modify Study tags`, and change:
  - `StudyDate = 20200101`
  - select "Modify the original study. (keeping the original DICOM UIDs)"
  - Apply
  - Check:
    - `StudyDate == 20200101`
    - `StudyInstanceUID` has not changed


Edit study tags, can not be used to edit a patient who already has other studies
- on the new Study (the one whose `PatientID==2`), `Modify Study tags` and change:
  - `PatientID = TEST_1`
  - don't change other Patient tags
  - try all 3 modification mode options
  - Apply
  - Check: you should get an error telling you to use `Change Patient`

Edit study tags, can be used to create a new patient
- on the new Study (the one whose `PatientID==2`), `Modify Study tags` and change:
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

Edit patient ID in multiple studies
- on a study from `PatientID = TEST_1`, 'Modify Patient tags' and set:
  - `PatientID = TEST_4`
  - select "Modify the original study. (keeping the original DICOM UIDs)"
  - Apply
  - Check:
    - `PatientID` has changed in all studies, 

Edit patient ID in multiple studies
- on a study from `PatientID = TEST_4`, 'Modify Patient tags' and set:
  - `PatientID = TEST_1`
  - select "Create a modified copy of the original study"
  - Apply
  - Check:
    - there are duplicate studies for Patient `TEST_1` and `TEST_4`

Anonymize study
- on any study, 'Anonymize study':
  - make sure `PatientID` and `PatientName` have been pre-filled
  - set a `StudyDescription`
  - Apply
  - Check:
    - the anonymized study has been created, the original study is still present


Series modification
------------------

Preparation:
- delete all studies from `TEST*` patient
- upload the test study again
- on Study `Test CT`, `Modify Study tags` and change:
  - `AccessionNumber = 2345`
  - `StudyDate = 20150101`
  - `StudyID = 2`
  - `StudyDescription = Test CT 2`
  - select "Create a modified copy of the original study"
  - Apply
- you should now have 2 studies for the `TEST_1` patient

Edit series tags to move it to an existing study:
- on the series from `Test CT 2`, `Change study` :
  - `StudyInstanceUID = 1.2.4`
  - click `Modify` -> you should get an error
  - `StudyInstanceUID = 1.2.3`
  - click `Modify`
  - Check:
    - you now have 2 series in the `Test CT` study
    - `StudyDate` tags of instances from both series should be set to `20100630`

Move series to a new study:
- on one of the series, `Create new study` and change:
  - `PatientID = TEST_1`
  - `StudyDescription = TEST CT 3`
  - click `Modify` -> you should have a warning telling you that this patient exists with different tags -> `Modify`
  - Check:
    - you should now have 2 studies for patient `TEST*`

Move series to a new study (2):
- on the series from `TEST CT 3`, `Create new study` and change:
  - `PatientID = TEST_2`
  - `PatientName = Test2`
  - `PatientBirthDate = 20000101`
  - `StudyDescription = TEST CT 4`
  - click `Modify`
  - Check:
    - you should now have 2 studies for patient `TEST*`
    - the `Test CT 3 ` series shall have been deleted

Edit series tags:
- on the series from `TEST CT 4`, `Modify series tags` and change:
  - `Modality = CR`
  - click `Modify`
  - Check:
    - the modality shall have been updated in the series (and in the study list)

Anonymize series:
- on the series from `TEST CT 4`, `Anonymize series` and change:
  - make sure `PatientID` and `PatientName` have been pre-filled
  - click `Anonymize`
  - Check:
    - the anonymized series (and so study) has been created, the original series is still present
