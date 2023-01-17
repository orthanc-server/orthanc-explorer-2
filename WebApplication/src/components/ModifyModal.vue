<script>
import { mapState } from "vuex"
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import api from "../orthancApi"


export default {
    props: ["orthancId", "studyMainDicomTags", "patientMainDicomTags"],
    methods: {
        modify() {
        }
    },
    data() {
        return {
            keepSource: "no",
            keepStudyInstanceUid: "no",
            tags: {},                                       // the tag values entered in the dialog
            originalTags: {},                              // the original tag values
            samePatientStudiesCount: 0,
            hasLoadedSamePatientsStudiesCount: false,
            areTagsModified: false,
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
            //            createNewStudy: true,
            //            showCreateNewStudyOption: true,
            step: 'init', // allowed values: 'init', 'tags', 'warning', 'error', 'progress', 'done'
            action: 'none'
        }
    },
    async mounted() {
        console.log("ModifyModal mounted");
        this.reset();
        // this.createNewStudy = ["Always", "DefaultYes"].indexOf(this.uiOptions.ModifyCreatesNewStudy) != -1;
        // this.showCreateNewStudyOption = ["DefaultYes", "DefaultNo"].indexOf(this.uiOptions.ModifyCreatesNewStudy) != -1;

        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
            this.reset();
        });
    },
    watch: {
        tags: {
            handler(newValue, oldValue) {
                console.log("tags changed", this.tags);
                this.areTagsModified = Object.keys(this.modifiedTags).length > 0;
            },
            deep: true
        },
        keepStudyInstanceUid(newValue, oldValue) {
            if (newValue == 'yes') {
                this.keepSource = 'no';
            }
        },
        keepSource(newValue, oldValue) {
            if (newValue == 'yes') {
                this.keepStudyInstanceUid = 'no';
            }
        },
    },
    methods: {
        async reset() {
            this.step = 'init';
            this.samePatientStudiesCount = (await api.getSamePatientStudies(this.patientMainDicomTags['PatientID'])).length;
            this.hasLoadedSamePatientsStudiesCount = true;

            this.tags = {};
            this.originalTags = {};
            this.modifiedTags = {};
            for (const [k, v] of Object.entries(this.patientMainDicomTags)) {
                this.originalTags[k] = v;
            }
            for (const [k, v] of Object.entries(this.studyMainDicomTags)) {
                this.originalTags[k] = v;
            }

            this.jobProgressComplete = 0;
            this.jobProgressFailed = 0;
            this.jobProgressRemaining = 100;
        },
        back() {
            if (this.step == 'tags') {
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
                if (jobStatus['Content']['Type'] == 'Patient') {
                    this.showModifiedResourceKey = "PatientID";
                    const modifiedPatient = await api.getPatient(jobStatus['Content']['ID']);
                    this.showModifiedResourceValue = modifiedPatient['MainDicomTags']['PatientID'];
                } else if (jobStatus['Content']['Type'] == 'Study') {
                    this.showModifiedResourceKey = "StudyInstanceUID";
                    const modifiedStudy = await api.getStudy(jobStatus['Content']['ID']);
                    this.showModifiedResourceValue = modifiedStudy['MainDicomTags']['StudyInstanceUID'];
                } else {
                    console.error("not handled yet")
                }
                // modifiedResourceFilter
            }
        },
        async modify() {
            // perform checks before modification
            if (this.action == 'attach-study-to-existing-patient') {
                // make sure we use an existing PatientID
                const targetPatient = await api.findPatient(this.tags['PatientID']);
                if (targetPatient) {
                    let replaceTags = targetPatient.MainDicomTags;
                    if (this.keepStudyInstanceUid) {
                        replaceTags['StudyInstanceUID'] = this.originalTags['StudyInstanceUID'];
                    }
                    const jobId = await api.modifyStudy(this.orthancId, replaceTags, [], this.keepSource == 'yes');
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
                let replaceTags = this.modifiedTags;
                if (this.keepStudyInstanceUid == 'yes') {
                    replaceTags['StudyInstanceUID'] = this.originalTags['StudyInstanceUID'];
                }
                const jobId = await api.modifyStudy(this.orthancId, replaceTags, [], this.keepSource == 'yes');
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
                const jobId = await api.modifyPatient(originalPatient['ID'], this.tags, [], this.keepSource == 'yes');
                console.log("modify-patient-tags-in-all-studies: created job ", jobId);
                this.startMonitoringJob(jobId);
            }
            // } else if (this.action == 'modify-study-tags') {
            //     // make sure we do not reuse an existing PatientID
            //     const targetPatient = await api.findPatient(this.tags['PatientID']);
            //     if (targetPatient) {
            //         console.error("Error while modifying study tags, another patient with the same PatientID already exists ", this.tags['PatientID'], targetPatient);
            //     } else {

            //         if (this.keepStudyInstanceUid) {
            //             replaceTags['StudyInstanceUID'] = this.originalTags['StudyInstanceUID'];
            //         }

            //         const jobId = await api.modifyStudy(this.orthancId, this.tags, []); // TODO: keepSource + keepStudyInstanceUID
            //         console.log("modify-study-tags created job ", jobId);
            //         this.startMonitoringJob(jobId);
            //     }
        },
        goToNextStep(step, action) {
            if (step == 'init') {
                this.step = 'tags';
                this.action = action;
                if (action == 'attach-study-to-existing-patient') {
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
                // } else if (action == 'modify-study-tags') {
                //     this.tags = {};
                //     for (const [k, v] of Object.entries(this.studyMainDicomTags)) {
                //         this.tags[k] = v;
                //     }
                } else if (action == 'modify-patient-tags-in-all-studies') {
                    this.tags = {};
                    for (const [k, v] of Object.entries(this.patientMainDicomTags)) {
                        this.tags[k] = v;
                    }
                } else {
                    console.error('unknown action ', action);
                }
            }
        },
        showModifiedResources() {
            let newUrl = "";
            if (this.showModifiedResourceKey) {
                newUrl = "/filtered-studies?" + this.showModifiedResourceKey + '=' + this.showModifiedResourceValue;
            }

            this.$router.replace(newUrl);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        resourceTitle() {
            return resourceHelpers.getResourceTitle("study", this.patientMainDicomTags, this.studyMainDicomTags, null, null);
        },
        modifiedTags() {
            let modifiedTags = {};
            for (const [k, v] of Object.entries(this.tags)) {
                if (this.tags[k] != this.originalTags[k]) {
                    modifiedTags[k] = this.tags[k];
                }
            }
            return modifiedTags;
        }
    },
    components: { CopyToClipboardButton }

};
</script>

<template>
    <div class="modal fade" :id="this.orthancId" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <!-- aria-hidden="true" -->
        <div class="modal-dialog modal-lg">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title" id="modalLabel">{{ $t("modify.modal_title") + " " + resourceTitle }} </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>

                <!-- ------------------------------------------ step 'init' --------------------------------------------------->
                <div v-if="step == 'init'" class="modal-body">
                    <div class="container">
                        <div class="row border-bottom pb-3">
                            <div class="col-md-9"
                                v-html="$t('modify.study_step_0_attach_study_to_existing_patient_html')">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.study_step_0_attach_study_to_existing_patient_button_title_html')"
                                    @click="goToNextStep(step, 'attach-study-to-existing-patient')"></button>
                            </div>
                        </div>
                        <div v-if="samePatientStudiesCount > 1" class="row border-bottom border-3 py-3">
                            <div class="col-md-9"
                                v-html="$t('modify.study_step_0_patient_has_other_studies_html', { count: samePatientStudiesCount })">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.study_step_0_patient_has_other_studies_button_title_html')"
                                    @click="goToNextStep(step, 'modify-patient-tags-in-all-studies')"></button>
                            </div>
                        </div>
                        <div v-if="samePatientStudiesCount >= 1" class="row pt-3">
                            <div class="col-md-9"
                                v-html="$t('modify.study_step_0_modify_study_html', { count: samePatientStudiesCount })">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.study_step_0_modify_study_button_title_html')"
                                    @click="goToNextStep(step, 'modify-any-tags-in-one-study')"></button>
                            </div>
                        </div>
                        <!-- <div v-if="samePatientStudiesCount > 1" class="row pt-3">
                            <div class="col-md-9"
                                v-html="$t('modify.study_step_0_study_step_0_modify_study_with_siblings_html', { count: samePatientStudiesCount })">
                            </div>
                            <div class="col-md-3">
                                <button type="button" class="btn btn-primary w-100"
                                    v-html="$t('modify.study_step_0_study_step_0_modify_study_with_siblings_button_title_html')"
                                    @click="goToNextStep(step, 'modify-study-tags')"></button>
                            </div>
                        </div> -->
                    </div>
                </div>
                <div v-if="step == 'init'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                </div>

                <!-- ------------------------------------------ step 'tags' --------------------------------------------------->
                <div v-if="step == 'tags'" class="modal-body">
                    <div class="container">
                        <div v-for="(item, key) in tags" :key="key" class="row">
                            <div class="col-md-5">
                                {{ key }}
                            </div>
                            <div class="col-md-6">
                                <input v-if="true" type="text" class="form-control" v-model="tags[key]"
                                    :placeholder="item" />
                            </div>
                            <!-- <div class="col-md-1">
                                <div class="d-flex flex-row-reverse">
                                    <CopyToClipboardButton :valueToCopy="item" />
                                </div>
                            </div> -->
                        </div>
                        <div class="row pt-3">
                            <div v-if="action=='modify-patient-tags-in-all-studies'" class="col-md-10">
                                {{ $t("modify.keep_source_studies") }}
                            </div>
                            <div v-if="action!='modify-patient-tags-in-all-studies'" class="col-md-10">
                                {{ $t("modify.keep_source_study") }}
                            </div>
                            <div class="col-md-1">
                                <label class="switch float-end">
                                    <input type="checkbox" v-model="keepSource" true-value="yes" false-value="no">
                                    <span class="slider round"></span>
                                </label>
                            </div>
                        </div>
                        <div v-if="action!='modify-patient-tags-in-all-studies'" class="row pt-3">
                            <div class="col-md-10">
                                {{ $t("modify.keep_study_instance_uid") }}
                            </div>
                            <div class="col-md-1">
                                <label class="switch float-end">
                                    <input type="checkbox" v-model="keepStudyInstanceUid" true-value="yes" false-value="no">
                                    <span class="slider round"></span>
                                </label>
                            </div>
                        </div>
                    </div>
                </div>
                <div v-if="step == 'tags'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="back()">{{
                        $t("modify.back_button_title")
                    }}</button>
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button type="button" class="btn btn-primary" :disabled="!areTagsModified" @click="modify()">{{
                        $t("modify.modify_button_title")
                    }}</button>
                </div>

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
                    <button type="button" class="btn btn-primary" @click="modify()">{{
                        $t("modify.modify_button_title")
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
</style>