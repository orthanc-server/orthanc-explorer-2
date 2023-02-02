<script>
import { mapState } from "vuex"
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import api from "../orthancApi"
import { v4 as uuidv4 } from "uuid"

// these tags can not be removed
document.requiredTags = [
    'PatientID', 'PatientBirthDate', 'PatientSex', , 'PatientName',
    'StudyInstanceUID', 'StudyDate', 'StudyTime', 'AccessionNumber', 'ReferringPhysicianName', 'StudyID',
    'SeriesInstanceUID', 'Modality', 'ContrastBolusAgent', 'SeriesNumber', 'ImageOrientationPatient', 'SeriesType'
];

// list of actions: 
// - 'attach-study-to-existing-patient', 
// - 'modify-any-tags-in-one-study',
// - 'modify-patient-tags-in-all-studies',
// - 'anonymize-study',
// - 'attach-series-to-existing-study',
// - 'modify-series-tags',
// - 'create-new-study-from-series',
// - 'anonymize-series'



export default {
    props: ["orthancId", "studyMainDicomTags", "patientMainDicomTags", "seriesMainDicomTags", "isAnonymization", "resourceLevel"],
    data() {
        return {
            tags: {},                                       // the tag values entered in the dialog
            originalTags: {},                              // the original tag values
            removedTags: {},
            insertedTags: new Set(),
            samePatientStudiesCount: 0,
            hasLoadedSamePatientsStudiesCount: false,
            warningMessageId: null,
            jobProgressComplete: 0,
            jobProgressFailed: 0,
            jobProgressRemaining: 100,
            jobRefreshTimeout: 200,
            jobIsComplete: false,
            jobIsRunning: false,
            jobIsSuccess: false,
            showModifiedResourceKey: null,
            showModifiedResourceValue: null,
            modificationMode: null,
            step: 'init', // allowed values: 'init', 'tags', 'warning', 'error', 'progress', 'done'
            action: 'none'
        }
    },
    async mounted() {
        this.reset();

        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
            this.reset();
        });
    },
    watch: {
    },
    methods: {
        async reset() {
            this.step = 'init';
            this.samePatientStudiesCount = (await api.getSamePatientStudies(this.patientMainDicomTags['PatientID'])).length;
            this.hasLoadedSamePatientsStudiesCount = true;

            this.tags = {};
            this.originalTags = {};
            this.removedTags = {};
            this.insertedTags = new Set();

            for (const [k, v] of Object.entries(this.patientMainDicomTags)) {
                this.originalTags[k] = v;
                this.removedTags[k] = false;
            }
            for (const [k, v] of Object.entries(this.studyMainDicomTags)) {
                this.originalTags[k] = v;
                this.removedTags[k] = false;
            }

            if (this.resourceLevel == "series") {
                for (const [k, v] of Object.entries(this.seriesMainDicomTags)) {
                    this.originalTags[k] = v;
                    this.removedTags[k] = false;
                }
            }

            this.jobProgressComplete = 0;
            this.jobProgressFailed = 0;
            this.jobProgressRemaining = 100;

            this.modificationMode = this.uiOptions.Modifications.DefaultMode;

            if (this.isAnonymization) {
                if (this.resourceLevel == 'study') {
                    this.goToNextStep('init', 'anonymize-study');
                } else {
                    this.goToNextStep('init', 'anonymize-series');
                }
            }
        },
        back() {
            if (this.step == 'tags' && !this.isAnonymization) {
                this.step = 'init';
            } else if (this.step == 'warning' || this.step == 'error') {
                this.step = 'tags';
            } else {
                console.error("unknown step for back function ", this.step);
            }
        },
        setWarning(warningMessageId) {
            this.warningMessageId = warningMessageId;
            this.step = 'warning';
        },
        setError(errorMessageId) {
            this.errorMessageId = errorMessageId;
            this.step = 'error';
        },
        startMonitoringJob(jobId) {
            this.step = 'progress';
            this.jobRefreshTimeout = 200;  // refresh quickly at the beginnning !
            setTimeout(this.monitorJob, this.jobRefreshTimeout, [jobId]);
        },
        async monitorJob(jobId) {
            const jobStatus = await api.getJobStatus(jobId);
            this.jobIsComplete = (jobStatus.State == "Success" || jobStatus.State == "Failure");
            this.jobIsRunning = (jobStatus.State == "Running");
            this.jobIsSuccess = (jobStatus.State == "Success");

            if (this.jobIsComplete) {

                this.jobProgressRemaining = 0;

                if (this.jobIsSuccess) {
                    this.jobProgressComplete = 100;
                    this.jobProgressFailed = 0;
                } else {
                    this.jobProgressComplete = 0;
                    this.jobProgressFailed = 100;
                }
            } else {
                this.jobProgressFailed = 0;
                this.jobProgressComplete = jobStatus.Progress;
                this.jobProgressRemaining = 100 - this.jobProgressComplete;
            }
            if (!this.jobIsComplete) {
                this.jobRefreshTimeout = Math.min(this.jobRefreshTimeout + 200, 2000);  // refresh quickly at the beginnning !
                setTimeout(this.monitorJob, this.jobRefreshTimeout, [jobId]);
            } else {
                this.step = 'done';
                const jobType = jobStatus['Type'];
                if (jobType == 'MergeStudy') {
                    const modifiedStudy = await api.getStudy(jobStatus['Content']['TargetStudy']);
                    this.showModifiedResourceValue = modifiedStudy['MainDicomTags']['StudyInstanceUID'];
                    this.showModifiedResourceKey = "StudyInstanceUID";
                } else if (jobType == 'ResourceModification') {
                    if (jobStatus['Content']['Type'] == 'Patient') {
                        this.showModifiedResourceKey = "PatientID";
                        const modifiedPatient = await api.getPatient(jobStatus['Content']['ID']);
                        this.showModifiedResourceValue = modifiedPatient['MainDicomTags']['PatientID'];
                    } else if (jobStatus['Content']['Type'] == 'Study') {
                        this.showModifiedResourceKey = "StudyInstanceUID";
                        const modifiedStudy = await api.getStudy(jobStatus['Content']['ID']);
                        this.showModifiedResourceValue = modifiedStudy['MainDicomTags']['StudyInstanceUID'];
                    } else if (jobStatus['Content']['Type'] == 'Series') {
                        this.showModifiedResourceKey = "StudyInstanceUID";
                        const modifiedStudy = await api.getSeriesParentStudy(jobStatus['Content']['ID']);
                        this.showModifiedResourceValue = modifiedStudy['MainDicomTags']['StudyInstanceUID'];
                    } else {
                        console.error("not handled yet")
                    }
                }
            }
        },
        async modify(hasAcceptedWarning = false) {
            try {
                // perform checks before modification
                if (this.action == 'attach-study-to-existing-patient') {
                    // make sure we use an existing PatientID
                    const targetPatient = await api.findPatient(this.tags['PatientID']);
                    if (targetPatient) {
                        const jobId = await api.modifyResource({
                            resourceLevel: 'study',
                            orthancId: this.orthancId,
                            replaceTags: targetPatient.MainDicomTags,
                            keepTags: (this.keepDicomUids ? ['StudyInstanceUID', 'SeriesInstanceUID', 'SOPInstanceUID'] : []),
                            keepSource: this.keepSource,
                            removeTags: this.removedTagsList
                        });
                        console.log("attach-study-to-existing-patient: created job ", jobId);
                        this.startMonitoringJob(jobId);
                    } else {
                        console.error("Error while changing patient, the new PatientID does not exist ", this.tags['PatientID']);
                        this.setError('modify.error_attach_study_to_existing_patient_target_does_not_exist_html');
                        return;
                    }
                } else if (this.action == 'modify-any-tags-in-one-study') {

                    if ('PatientID' in this.modifiedTags) {
                        console.log("modify-any-tags-in-one-study: PatientID has changed");
                        const targetPatient = await api.findPatient(this.tags['PatientID']);
                        if (targetPatient) {
                            console.error("modify-any-tags-in-one-study: Error while modifying patient-study tags, another patient with the same PatientID already exists ", this.tags['PatientID'], targetPatient);
                            this.setError('modify.error_modify_any_study_tags_patient_exists_html');
                            return;
                        }
                    } else if ('PatientName' in this.modifiedTags || 'PatientBirthDate' in this.modifiedTags || 'PatientSex' in this.modifiedTags) {
                        console.log("modify-any-tags-in-one-study: Patient tags have changed");
                        this.samePatientStudiesCount = (await api.getSamePatientStudies(this.originalTags['PatientID'])).length;
                        if (this.samePatientStudiesCount > 1) {
                            console.error("modify-any-tags-in-one-study: Error while modifying patient-study tags, this patient has other studies, you can not modify patient tags ", this.originalTags['PatientID'], this.modifiedTags);
                            this.setError('modify.error_modify_any_study_tags_can_not_modify_patient_tags_because_of_other_studies_html');
                            return;
                        } else {
                            console.log("modify-any-tags-in-one-study: this is the only patient study, we can modify all tags");
                        }
                    }

                    // at this point, it is safe to modify any tags
                    const jobId = await api.modifyResource({
                        resourceLevel: 'study',
                        orthancId: this.orthancId,
                        replaceTags: this.modifiedTags,
                        keepTags: (this.keepDicomUids ? ['StudyInstanceUID', 'SeriesInstanceUID', 'SOPInstanceUID'] : []),
                        keepSource: this.keepSource,
                        removeTags: this.removedTagsList
                    });
                    console.log("modify-any-tags-in-one-study: created job ", jobId);
                    this.startMonitoringJob(jobId);

                } else if (this.action == 'modify-patient-tags-in-all-studies') {
                    // if we try to change the PatientID, make sure we do not reuse an existing PatientID
                    if (this.tags['PatientID'] != this.originalTags['PatientID']) {
                        const targetPatient = await api.findPatient(this.tags['PatientID']);
                        if (targetPatient) {
                            console.error("modify-patient-tags-in-all-studies: Error while modifying patient tags, another patient with the same PatientID already exists ", this.tags['PatientID'], targetPatient);
                            this.setError('modify.error_modify_patient_tags_another_patient_exists_with_same_patient_id_html');
                            return;
                        }
                    }

                    const originalPatient = await api.findPatient(this.originalTags['PatientID']);
                    const jobId = await api.modifyResource({
                        resourceLevel: 'patient',
                        orthancId: originalPatient['ID'],
                        replaceTags: this.tags,
                        keepTags: (this.keepDicomUids ? ['StudyInstanceUID', 'SeriesInstanceUID', 'SOPInstanceUID'] : []),
                        removeTags: this.removedTagsList,
                        keepSource: this.keepSource
                    });
                    console.log("modify-patient-tags-in-all-studies: created job ", jobId);
                    this.startMonitoringJob(jobId);
                } else if (this.action == 'anonymize-study') {
                    const jobId = await api.anonymizeResource({
                        resourceLevel: 'study',
                        orthancId: this.orthancId,
                        replaceTags: this.tags,
                        removeTags: this.removedTagsList
                    });
                    console.log("anonymize-study: created job ", jobId);
                    this.startMonitoringJob(jobId);
                } else if (this.action == 'attach-series-to-existing-study') {
                    if (this.tags['StudyInstanceUID'] == this.originalTags['StudyInstanceUID']) {
                        console.error("attach-series-to-existing-study: Error target study is same as current study ", this.tags['StudyInstanceUID']);
                        this.setError('modify.error_attach_series_to_existing_study_target_unchanged_html');
                        return;
                    } else {
                        const targetStudy = await api.findStudy(this.tags['StudyInstanceUID']);
                        if (targetStudy == null) {
                            console.error("attach-series-to-existing-study: target study not found ", this.tags['StudyInstanceUID']);
                            this.setError('modify.error_attach_series_to_existing_study_target_does_not_exist_html');
                            return;
                        } else {
                            const jobId = await api.mergeSeriesInExistingStudy({
                                seriesIds: [this.orthancId],
                                targetStudyId: targetStudy['ID'],
                                keepSource: false
                            });
                            console.log("attach-series-to-existing-study: created job ", jobId);
                            this.startMonitoringJob(jobId);                            
                        }
                    }
                } else if (this.action == 'create-new-study-from-series') {
                    // check if a patient with the same PatientID already exists
                    const targetPatient = await api.findPatient(this.tags['PatientID']);
                    if (targetPatient) {
                        if (!hasAcceptedWarning) {
                            if (targetPatient['MainDicomTags']['PatientName'] != this.tags['PatientName']
                                || targetPatient['MainDicomTags']['PatientBirthDate'] != this.tags['PatientBirthDate']
                                || targetPatient['MainDicomTags']['PatientSex'] != this.tags['PatientSex']
                            ) {
                                console.warn("create-new-study-from-series: Another patient exists but tags differ", targetPatient['MainDicomTags'], this.tags);
                                this.setWarning('modify.warning_create_new_study_from_series_html');
                                return;
                            }
                        } else {
                            console.log("create-new-study-from-series: warning accepted, copying existing Patient tags");
                            this.tags['PatientName'] = targetPatient['MainDicomTags']['PatientName'];
                            this.tags['PatientBirthDate'] = targetPatient['MainDicomTags']['PatientBirthDate'];
                            this.tags['PatientSex'] = targetPatient['MainDicomTags']['PatientSex'];
                        }
                    }

                    // generate a new StudyInstanceUID (since we perform modification at series level, orthanc would keep even if not listed in keepTags)
                    this.tags['StudyInstanceUID'] = await api.generateUid('study');
                    const jobId = await api.modifyResource({
                        resourceLevel: 'series',
                        orthancId: this.orthancId,
                        replaceTags: this.tags,
                        keepTags: ['SeriesInstanceUID', 'SOPInstanceUID'],
                        keepSource: false
                    });
                    console.log("create-new-study-from-series: created job ", jobId);
                    this.startMonitoringJob(jobId);                            

                } else if (this.action == 'modify-series-tags') {
                    const jobId = await api.modifyResource({
                        resourceLevel: 'series',
                        orthancId: this.orthancId,
                        replaceTags: this.tags,
                        keepTags: ['SeriesInstanceUID', 'SOPInstanceUID'],
                        keepSource: true   // keep the source since the orthanc id won't change !
                    });
                    console.log("modify-series-tags: created job ", jobId);
                    this.startMonitoringJob(jobId);                            
                } else if (this.action == 'anonymize-series') {
                    const jobId = await api.anonymizeResource({
                        resourceLevel: 'series',
                        orthancId: this.orthancId,
                        replaceTags: this.tags,
                        removeTags: this.removedTagsList
                    });
                    console.log("anonymize-study: created job ", jobId);
                    this.startMonitoringJob(jobId);
                } else {
                    console.error('unhandled');
                }
            } catch (err) {
                this.setError('modify.error_modify_unexpected_error_html');
            }
        },
        goToNextStep(step, action) {
            if (step == 'init') {
                this.step = 'tags';
                this.action = action;
                if (action == 'anonymize-study') {
                    const uuid = uuidv4();

                    this.tags = {};
                    this.tags['PatientID'] = uuid;
                    this.tags['PatientName'] = 'Anonymized ' + uuid.substr(0, 8);
                    this.tags['PatientBirthDate'] = '';
                    this.tags['PatientSex'] = '';
                    if ('StudyDescription' in this.studyMainDicomTags) {
                        this.tags['StudyDescription'] = this.studyMainDicomTags['StudyDescription'];
                    }
                } else if (action == 'attach-study-to-existing-patient') {
                    this.tags = {};
                    this.tags['PatientID'] = this.patientMainDicomTags['PatientID'];
                } else if (action == 'modify-any-tags-in-one-study') {
                    this.tags = {};
                    for (const [k, v] of Object.entries(this.patientMainDicomTags)) {
                        this.tags[k] = v;
                    }
                    for (const [k, v] of Object.entries(this.studyMainDicomTags)) {
                        this.tags[k] = v;
                    }
                } else if (action == 'modify-patient-tags-in-all-studies') {
                    this.tags = {};
                    for (const [k, v] of Object.entries(this.patientMainDicomTags)) {
                        this.tags[k] = v;
                    }
                } else if (action == 'attach-series-to-existing-study') {
                    this.tags = {};
                    this.tags['StudyInstanceUID'] = "";
                } else if (action == 'create-new-study-from-series') {
                    this.tags = {};
                    this.tags['PatientID'] = '';
                    this.tags['PatientName'] = '';
                    this.tags['PatientBirthDate'] = '';
                    this.tags['PatientSex'] = '';
                    this.tags['StudyDescription'] = '';
                } else if (action == 'modify-series-tags') {
                    this.tags = {};
                    for (const [k, v] of Object.entries(this.seriesMainDicomTags)) {
                        this.tags[k] = v;
                    }
                } else if (action == 'anonymize-series') {
                    const uuid = uuidv4();

                    this.tags = {};
                    this.tags['PatientID'] = uuid;
                    this.tags['PatientName'] = 'Anonymized ' + uuid.substr(0, 8);
                    this.tags['PatientBirthDate'] = '';
                    this.tags['PatientSex'] = '';
                    if ('StudyDescription' in this.studyMainDicomTags) {
                        this.tags['StudyDescription'] = this.studyMainDicomTags['StudyDescription'];
                    }
                } else {
                    console.error('unknown action ', action);
                }
            } else if (this.step == 'warning') {
                this.modify();
            }
        },
        async showModifiedResources() {
            let newUrl = "";
            let query = {
                'forceRefresh' : Date.now() // to force refresh
            };
            if (this.showModifiedResourceKey) {
                query[this.showModifiedResourceKey] = this.showModifiedResourceValue;
            }

            this.$router.push({path: newUrl, query: query});
            // location.reload();
        },
        isFrozenTag(tag) {
            if (this.isRemovedTag(tag)) {
                return true;
            }
            if (this.resourceLevel == 'series') {
                if (this.action == 'modify-series-tags' && tag == 'SeriesInstanceUID') {
                    return true;
                } else {
                    return false;
                }
            }
            else if (this.isDicomUid(tag)) {
                return this.keepDicomUids;
            } else {
                return false;
            }
        },
        isEditableTag(tag) {
            if (this.isRemovedTag(tag)) {
                return false;
            }
            if (this.resourceLevel == 'series') {
                if (this.action == 'attach-series-to-existing-study' && tag == 'StudyInstanceUID') {
                    return true;
                }
            }

            if (this.isDicomUid(tag)) {
                return false;
            }
            return true;
        },
        isDicomUid(tag) {
            return ['StudyInstanceUID', 'SeriesInstanceUID', 'SOPInstanceUID'].indexOf(tag) != -1;
        },
        isAutogeneratedDicomUid(tag) {
            if (this.resourceLevel == 'series') {
                return false;
            }
            if (this.isDicomUid(tag)) {
                return !this.keepDicomUids;
            }
            return false;
        },
        isRemovable(tag) {

            if (document.requiredTags.indexOf(tag) != -1) {
                return false;
            } else {
                return true;
            }
        },
        isRemovedTag(tag) {
            if (this.removedTags[tag] === undefined) {
                console.warn("isRemovedTag: undefined tag");
            }
            return this.removedTags[tag];
        },
        toggleRemovedTag(tag) {
            this.removedTags[tag] = !this.removedTags[tag];
            if (this.removedTags[tag] && this.insertedTags.has(tag)) {
                // the tag has been inserted, remove it completely
                delete this.tags[tag];
                delete this.originalTags[tag];
                delete this.removedTags[tag];
                this.insertedTags.delete(tag);
            }

            if (this.removedTags) {
                console.log(tag + " will be removed")
            } else {
                console.log(tag + " will not be removed")
            }
        },
        insertTag(ev, tag) {
            ev.preventDefault();
            ev.stopPropagation();

            console.log("insert tag " + tag, this.tags, this.removedTags);
            this.tags[tag] = "";
            this.removedTags[tag] = false;
            this.originalTags[tag] = null;
            this.insertedTags.add(tag);
            console.log("insert tag " + tag, this.tags, this.removedTags);
        },
        isModeAllowed(mode) {
            if (this.resourceLevel == 'series') {
                return false;
            }
            return this.uiOptions.Modifications.AllowedModes.indexOf(mode) != -1;
        }

    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions
        }),
        resourceTitle() {
            return resourceHelpers.getResourceTitle(this.resourceLevel, this.patientMainDicomTags, this.studyMainDicomTags, this.seriesMainDicomTags, null);
        },
        keepSource() {
            if (this.modificationMode == "duplicate") {
                return true;
            } else if (this.modificationMode == "modify-new-uids") {
                return false;
            } else if (this.modificationMode == "modify-keep-uids") {
                if (this.action == "attach-study-to-existing-patient") { // PatientID changes -> orthancID changes -> delete original
                    return false;
                } else if (this.action == "modify-any-tags-in-one-study" || this.action == "modify-patient-tags-in-all-studies") {
                    if ('PatientID' in this.modifiedTags) {
                        return false; // PatientID changes -> orthancID changes -> delete original
                    } else {
                        return true; // PatientID does not change -> orthancID does not change -> do not delete original
                    }
                } else {
                    console.error("unhandled");
                }
            } else {
                console.error("unhandled");
            }
        },
        keepDicomUids() {
            if (this.modificationMode == "modify-keep-uids") {
                return true;
            } else {
                return false;
            }
        },
        areTagsModified() {
            let areTagsRemoved = false;
            for (const [k, v] of Object.entries(this.removedTags)) {
                if (v) {
                    areTagsRemoved = true;
                }
            }
            return (Object.keys(this.modifiedTags).length > 0 || areTagsRemoved)
        },
        modifiedTags() {
            let modifiedTags = {};
            for (const [k, v] of Object.entries(this.tags)) {
                if (this.tags[k] != this.originalTags[k] && !this.removedTags[k]) {
                    modifiedTags[k] = this.tags[k];
                }
            }
            return modifiedTags;
        },
        insertableTags() {
            let tags = [];
            if (this.action == 'modify-any-tags-in-one-study') {
                tags = tags.concat(this.uiOptions.StudyMainTags);
                tags = tags.concat(this.uiOptions.PatientMainTags);
            }
            const that = this;
            tags = tags.filter(function (value, index, arr) {
                return !(value in that.originalTags);
            })

            return tags.sort();
        },
        removedTagsList() {
            let l = [];
            for (const [k, v] of Object.entries(this.removedTags)) {
                if (v) {
                    l.push(k);
                }
            }
            return l;
        },
        modifyButtonTitle() {
            if (this.isAnonymization) {
                return 'modify.anonymize_button_title';
            } else {
                return 'modify.modify_button_title';
            }
        },
        modifyButtonEnabled() {
            if (this.isAnonymization) {
                return true;
            } else {
                return this.areTagsModified;
            }
        }
    },
    components: { CopyToClipboardButton }

};
</script>

<template>
    <div class="modal fade" :id="this.orthancId" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <!-- aria-hidden="true" -->
        <div class="modal-dialog modal-lg modal-dialog-scrollable">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 v-if="!isAnonymization" class="modal-title" id="modalLabel">{{ $t("modify.modify_modal_title") + " " + resourceTitle }} </h5>
                    <h5 v-if="isAnonymization" class="modal-title" id="modalLabel">{{ $t("modify.anonymize_modal_title") + " " + resourceTitle }} </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>

                <!-- ------------------------------------------ step 'init' --------------------------------------------------->
                <div v-if="step == 'init'" class="modal-body">
                    <div class="container">
                        <div v-if="resourceLevel=='study' && !isAnonymization" class="row border-bottom pb-3">
                            <div class="col-md-9"
                                v-html="$t('modify.study_step_0_attach_study_to_existing_patient_html')">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.study_step_0_attach_study_to_existing_patient_button_title_html')"
                                    @click="goToNextStep(step, 'attach-study-to-existing-patient')"></button>
                            </div>
                        </div>
                        <div v-if="resourceLevel=='study' && !isAnonymization && samePatientStudiesCount > 1" class="row border-bottom border-3 py-3">
                            <div class="col-md-9"
                                v-html="$t('modify.study_step_0_patient_has_other_studies_html', { count: samePatientStudiesCount })">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.study_step_0_patient_has_other_studies_button_title_html')"
                                    @click="goToNextStep(step, 'modify-patient-tags-in-all-studies')"></button>
                            </div>
                        </div>
                        <div v-if="resourceLevel=='study' && !isAnonymization && samePatientStudiesCount >= 1" class="row pt-3">
                            <div class="col-md-9"
                                v-html="$t('modify.study_step_0_modify_study_html', { count: samePatientStudiesCount })">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.study_step_0_modify_study_button_title_html')"
                                    @click="goToNextStep(step, 'modify-any-tags-in-one-study')"></button>
                            </div>
                        </div>
                        <div v-if="resourceLevel=='series' && !isAnonymization" class="row border-bottom pb-3">
                            <div class="col-md-9"
                                v-html="$t('modify.series_step_0_attach_series_to_existing_study_html')">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.series_step_0_attach_series_to_existing_study_button_title_html')"
                                    @click="goToNextStep(step, 'attach-series-to-existing-study')"></button>
                            </div>
                        </div>
                        <div v-if="resourceLevel=='series' && !isAnonymization" class="row border-bottom border-3 py-3">
                            <div class="col-md-9"
                                v-html="$t('modify.series_step_0_create_new_study_html')">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.series_step_0_create_new_study_button_title_html')"
                                    @click="goToNextStep(step, 'create-new-study-from-series')"></button>
                            </div>
                        </div>
                        <div v-if="resourceLevel=='series' && !isAnonymization" class="row pt-3">
                            <div class="col-md-9"
                                v-html="$t('modify.series_step_0_modify_series_html')">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.series_step_0_modify_series_button_title_html')"
                                    @click="goToNextStep(step, 'modify-series-tags')"></button>
                            </div>
                        </div>
                    </div>
                </div>
                <div v-if="step == 'init'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                </div>

                <!-- ------------------------------------------ step 'tags' --------------------------------------------------->
                <div v-if="step == 'tags'" class="modal-body">
                    <div class="container">
                        <div v-for="(item, key) in tags" :key="key" class="row">
                            <!----  label  ---->
                            <div class="col-md-5" :class="{ 'striked-through': isRemovedTag(key) }">
                                {{ key }}
                            </div>

                            <!----  edit text  ---->
                            <div v-if="isFrozenTag(key)" class="col-md-6">
                                <input v-if="true" type="text" class="form-control" disabled :class="{ 'striked-through': !isDicomUid(key) }"
                                    v-model="originalTags[key]" />
                            </div>
                            <div v-if="isEditableTag(key)" class="col-md-6">
                                <input v-if="true" type="text" class="form-control" v-model="tags[key]" />
                            </div>

                            <div v-if="isAutogeneratedDicomUid(key)" class="col-md-6">
                                <input v-if="true" type="text" class="form-control" disabled
                                    :value="$t('modify.autogenerated_dicom_uid')" />
                            </div>

                            <!----  buttons  ---->
                            <div v-if="true" class="col-md-1">
                                <div class="d-flex flex-row-reverse">
                                    <button v-if="isRemovable(key) && !isRemovedTag(key)" class="btn-small"
                                        type="button" data-bs-toggle="tooltip" :title="$t('modify.remove_tag_tooltip')"
                                        @click="toggleRemovedTag(key)">
                                        <i v-if="!isRemovedTag(key)" class="bi-trash"></i>
                                    </button>
                                    <button v-if="isRemovable(key) && isRemovedTag(key)" class="btn-small" type="button"
                                        data-bs-toggle="tooltip" :title="$t('modify.remove_tag_undo_tooltip')"
                                        @click="toggleRemovedTag(key)">
                                        <i v-if="isRemovedTag(key)" class="bi-recycle"></i>
                                    </button>
                                </div>
                            </div>

                            <div v-if="isAutogeneratedDicomUid(key)" class="col-md-1">
                            </div>

                            <!-- <div v-if="isFrozenTag(key)" class="col-md-6">
                                <input v-if="true" type="text" class="form-control" disabled :class="{ 'striked-through': !isDicomUid(key) }"
                                    v-model="originalTags[key]" />
                            </div>
                            <div v-if="isFrozenTag(key)" class="col-md-1">
                                <div class="d-flex flex-row-reverse">
                                    <button v-if="isRemovable(key) && !isRemovedTag(key)" class="btn-small"
                                        type="button" data-bs-toggle="tooltip" :title="$t('modify.remove_tag_tooltip')"
                                        @click="toggleRemovedTag(key)">
                                        <i v-if="!isRemovedTag(key)" class="bi-trash"></i>
                                    </button>
                                    <button v-if="isRemovable(key) && isRemovedTag(key)" class="btn-small" type="button"
                                        data-bs-toggle="tooltip" :title="$t('modify.remove_tag_undo_tooltip')"
                                        @click="toggleRemovedTag(key)">
                                        <i v-if="isRemovedTag(key)" class="bi-recycle"></i>
                                    </button>
                                </div>
                            </div>
                            <div v-if="!isFrozenTag(key) && isEditableTag(key)" class="col-md-6">
                                <input v-if="true" type="text" class="form-control" v-model="tags[key]" />
                            </div>
                            <div v-if="!isFrozenTag(key) && isEditableTag(key)" class="col-md-1">
                                <div class="d-flex flex-row-reverse">
                                    <button v-if="isRemovable(key) && !isRemovedTag(key)" class="btn-small"
                                        type="button" data-bs-toggle="tooltip" :title="$t('modify.remove_tag_tooltip')"
                                        @click="toggleRemovedTag(key)">
                                        <i v-if="!isRemovedTag(key)" class="bi-trash"></i>
                                    </button>
                                    <button v-if="isRemovable(key) && isRemovedTag(key)" class="btn-small" type="button"
                                        data-bs-toggle="tooltip" :title="$t('modify.remove_tag_undo_tooltip')"
                                        @click="toggleRemovedTag(key)">
                                        <i v-if="isRemovedTag(key)" class="bi-recycle"></i>
                                    </button>
                                </div>
                            </div> -->

                        </div>
                        <div v-if="action == 'modify-any-tags-in-one-study'" class="row pt-3">
                            <div class="col-md-8"></div>
                            <div class="col-md-2">
                                <div class="dropdown">
                                    <button class="btn btn-sm btn-secondary dropdown-toggle" type="button"
                                        id="dropdownInsertTag" data-bs-toggle="dropdown" aria-expanded="false">
                                        {{ $t('modify.insert_tag') }}
                                    </button>
                                    <ul class="dropdown-menu">
                                        <li v-for="(key, i) in insertableTags" :key="key" :value="key"><a
                                                class="dropdown-item" href="#" @click="insertTag($event, key)">{{ key }}</a>
                                        </li>
                                    </ul>
                                </div>
                            </div>
                        </div>
                        <div v-if="!isAnonymization" class="row pt-3">
                            <div v-if="isModeAllowed('modify-new-uids')" class="form-check">
                                <input class="form-check-input" type="radio" name="modificationMode" id="modifyNewUids"
                                    value="modify-new-uids" v-model="modificationMode">
                                <label class="form-check-label" for="modifyNewUids"
                                    v-html="$t('modify.modify_mode_new_uids_html')">
                                </label>
                            </div>
                            <div v-if="isModeAllowed('modify-keep-uids')" class="form-check">
                                <input class="form-check-input" type="radio" name="modificationMode" id="modifyKeepUids"
                                    value="modify-keep-uids" v-model="modificationMode">
                                <label class="form-check-label" for="modifyKeepUids"
                                    v-html="$t('modify.modify_mode_keep_uids_html')">
                                </label>
                            </div>
                            <div v-if="isModeAllowed('duplicate')" class="form-check">
                                <input class="form-check-input" type="radio" name="modificationMode"
                                    id="modifyDuplicate" value="duplicate" v-model="modificationMode">
                                <label class="form-check-label" for="modifyDuplicate"
                                    v-html="$t('modify.modify_mode_duplicate_html')">
                                </label>
                            </div>
                        </div>
                    </div>
                </div>
                <div v-if="step == 'tags'" class="modal-footer">
                    <button v-if="!isAnonymization" type="button" class="btn btn-secondary" @click="back()">{{
                        $t("modify.back_button_title")
                    }}</button>
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button type="button" class="btn btn-primary" :disabled="!modifyButtonEnabled" @click="modify()">{{
                        $t(modifyButtonTitle)
                    }}</button>
                </div>
                <!-- <div v-if="step == 'tags' && isAnonymization" class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button type="button" class="btn btn-primary" @click="modify()">{{
                        $t("modify.anonymize_button_title")
                    }}</button>
                </div> -->

                <!-- ------------------------------------------ step 'warning' or 'error' --------------------------------------------------->
                <div v-if="step == 'warning' || step == 'error'" class="modal-body">
                    <div class="container">
                        <div v-if="step == 'error'" class="alert alert-danger" role="alert" v-html="$t(errorMessageId)">
                        </div>
                        <div v-if="step == 'warning'" class="alert alert-warning" role="alert"
                            v-html="$t(warningMessageId)">
                        </div>
                    </div>
                </div>
                <div v-if="step == 'warning'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="back()">{{
                        $t("modify.back_button_title")
                    }}</button>
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button type="button" class="btn btn-primary" @click="modify(true)">{{
                        $t(modifyButtonTitle)
                    }}</button>
                </div>
                <div v-if="step == 'error'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="back()">{{
                        $t("modify.back_button_title")
                    }}</button>
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                </div>

                <!-- ------------------------------------------ step 'progress' and 'done' --------------------------------------------------->

                <div v-if="step == 'progress' || step == 'done'" class="modal-body">
                    <div class="container">
                        <div class="row pb-3">{{ $t('modify.job_in_progress') }}</div>
                        <div class="row">
                            <div class="progress" style="width:100%">
                                <div class="progress-bar bg-success" role="progressbar"
                                    v-bind:style="'width: ' + this.jobProgressComplete + '%'"></div>
                                <div class="progress-bar bg-secondary" role="progressbar"
                                    v-bind:style="'width: ' + this.jobProgressRemaining + '%'"></div>
                                <div class="progress-bar bg-danger" role="progressbar"
                                    v-bind:style="'width: ' + this.jobProgressFailed + '%'"></div>
                            </div>
                        </div>
                    </div>
                </div>
                <div v-if="step == 'progress' || step == 'done'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("close") }}</button>
                    <button type="button" class="btn btn-primary" :disabled="step != 'done'" data-bs-dismiss="modal"
                        @click="showModifiedResources()">{{ $t("modify.show_modified_resources") }}</button>
                </div>

            </div>
        </div>
    </div>
</template>

<style>
/* The switch - the box around the slider */
.switch {
    position: relative;
    display: inline-block;
    width: 60px;
    height: 34px;
}

/* Hide default HTML checkbox */
.switch input {
    opacity: 0;
    width: 0;
    height: 0;
}

/* The slider */
.slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #ccc;
    -webkit-transition: .4s;
    transition: .4s;
}

.slider:before {
    position: absolute;
    content: "";
    height: 26px;
    width: 26px;
    left: 4px;
    bottom: 4px;
    background-color: white;
    -webkit-transition: .4s;
    transition: .4s;
}

input:checked+.slider {
    background-color: #2196F3;
}

input:focus+.slider {
    box-shadow: 0 0 1px #2196F3;
}

input:checked+.slider:before {
    -webkit-transform: translateX(26px);
    -ms-transform: translateX(26px);
    transform: translateX(26px);
}

/* Rounded sliders */
.slider.round {
    border-radius: 34px;
}

.slider.round:before {
    border-radius: 50%;
}

.disabled-text {
    color: #888;
}

.btn-small {
    line-height: var(--bs-body-line-height);
    border: 0;
    background-color: transparent;
    border-radius: 0.2rem;
}

.striked-through {
    text-decoration: line-through;
}
</style>