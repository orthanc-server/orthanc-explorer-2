<script>
import Modal from "./Modal.vue"
import { mapState } from "vuex"

export default {
    props: ["report", "showStudyDetails"],
    emits: ["deletedUploadReport"],
    data() {
        return {
        };
    },
    mounted() {

    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        pctSuccess() {
            return 100.0 * this.report.successFilesCount / this.report.filesCount;
        },
        pctSkipped() {
            return 100.0 * this.report.skippedFilesCount / this.report.filesCount;
        },
        pctFailed() {
            return 100.0 * this.report.failedFilesCount / this.report.filesCount;
        },
        pctRemaining() {
            return 100.0 * (this.report.filesCount - (this.report.successFilesCount + this.report.skippedFilesCount + this.report.failedFilesCount) ) / this.report.filesCount;
        },
        progressSuccessText() {
            if (this.pctSuccess >= Math.max(this.pctRemaining, this.pctSkipped, this.pctFailed)) {
                return this.progressText;
            } else {
                return "";
            }
        },
        progressSkippedText() {
            if (this.pctSkipped > Math.max(this.pctRemaining, this.pctSuccess, this.pctFailed)) {
                return this.progressText;
            } else {
                return "";
            }
        },
        progressFailedText() {
            if (this.pctFailed > Math.max(this.pctRemaining, this.pctSkipped, this.pctSuccess)) {
                return this.progressText;
            } else {
                return "";
            }
        },
        progressRemainingText() {
            if (this.pctRemaining > Math.max(this.pctSuccess, this.pctSkipped, this.pctFailed)) {
                return this.progressText;
            } else {
                return "";
            }
        },
        progressText() {
            if (this.report.filesCount > 0) {
                return (this.report.successFilesCount + this.report.skippedFilesCount + this.report.failedFilesCount) + " / " + this.report.filesCount;
            }
        },
        uploadedStudiesCount() {
            return this.report.uploadedStudiesIds.size;
        }
    },
    methods: {
        close(reportId) {
            this.$emit("deletedUploadReport", reportId);
        },
        getStudyLine(studyId, studyMainDicomTags, patientMainDicomTags) {
            // format the line to display for each study
            let infos = [];

            for (let tag of this.uiOptions.UploadReportTags) {
                if (tag in studyMainDicomTags && studyMainDicomTags[tag] && studyMainDicomTags[tag].length > 0) {
                    infos.push(studyMainDicomTags[tag]);
                } else if (tag in patientMainDicomTags && patientMainDicomTags[tag] && patientMainDicomTags[tag].length > 0) {
                    infos.push(patientMainDicomTags[tag]);
                }
            }
            if (infos.length == 0) { // if nothing to display, display the study id
                infos.push(studyId.slice(0, 20) + "...");
            }   
            return infos.slice(0, this.uiOptions.UploadReportMaxTags).join(" - ");
        }        
    },
    components: { Modal }
}
</script>

<template>
    <div class="card border-secondary job-card">
        <div class="card-header jobs-header">
            {{ $t('upload') }} {{ report.filesCount }} {{ $t('files') }}
            <button type="button" class="btn-close job-card-close" aria-label="Close"
                @click="close(report.id)"></button>
            <div class="progress mt-1 mb-1" style="width:90%">
                <div class="progress-bar bg-success" role="progressbar"
                    v-bind:style="'width: ' + this.pctSuccess + '%'">{{ progressSuccessText }}</div>
                <div class="progress-bar bg-secondary" role="progressbar"
                    v-bind:style="'width: ' + this.pctSkipped + '%'">{{ progressSkippedText }}</div>
                <div class="progress-bar bg-danger" role="progressbar" v-bind:style="'width: ' + this.pctFailed + '%'">
                    {{ progressFailedText }}
                </div>
                {{ progressRemainingText }}
            </div>
            <div v-show="Object.keys(report.errorMessages).length > 0">
                <button class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                    v-bind:data-bs-target="'#upload-errors-modal-' + this.report.id">{{ $t('show_errors') }}</button>
                <Modal :id="'upload-errors-modal-' + this.report.id" :headerText="'Upload error report'"
                    :okText="'Close'" @ok="close($event)">
                    <template #modalBody>
                        <table class="table table-sm">
                            <thead>
                                <tr>
                                    <th width="50%">{{ $t('file') }}</th>
                                    <th width="50%">{{ $t('error') }}</th>
                                </tr>
                            </thead>
                            <tbody>
                                <tr v-for="(error, file) in report.errorMessages" :key="error">
                                    <td>{{ file }}</td>
                                    <td class="text-right">{{ error }}</td>
                                </tr>
                            </tbody>
                        </table>
                    </template>
                </Modal>
            </div>
        </div>
        <div class="card-body text-secondary jobs-body">
            <p v-if="showStudyDetails" class="card-text">
                <span class="upload-details">{{ $t('uploaded_studies') }}:</span>
                <br />
                <span v-for="(study, studyId) in report.uploadedStudies" :key="studyId">
                    <router-link
                        v-bind:to="'/filtered-studies?StudyInstanceUID=' + study.MainDicomTags['StudyInstanceUID'] + '&expand=study'"
                        class="upload-details-study">{{ this.getStudyLine(studyId, study.MainDicomTags,
                                study.PatientMainDicomTags)
                        }}</router-link>
                    <br />
                </span>
            </p>
            <p v-if="!showStudyDetails" class="card-text">
                <span class="upload-details">{{ $t('uploaded_count_studies', {'count': uploadedStudiesCount }) }}</span>
            </p>
        </div>
    </div>
</template>
<style scoped>
</style>