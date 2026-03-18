<script>
import Modal from "./Modal.vue"
import { mapState } from "vuex"
import api from "../orthancApi"

export default {
    props: ["job"],
    emits: ["deletedJob"],
    data() {
        return {
            isComplete: false,
            isRunning: false,
            isSuccess: false,
            pctComplete: 0,
            pctFailed: 0,
            pctRemaining: 100,
            refreshTimeout: 200,
            showDetails: true,
            resources: {},
            jobDetailsTitle: null
        };
    },
    mounted() {

    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        statusText() {
            if (this.isSuccess) {
                return this.$t('job_success');
            } else if (this.isComplete) {
                return this.$t('job_failure');
            }
            return "";
        },
        shouldShowDetails() {
            return this.showDetails && Object.keys(this.resources).length > 0;
        }
    },
    methods: {
        close(jobId) {
            this.$emit("deletedJob", jobId);
        },
        async refresh(jobId) {
            const jobStatus = await api.getJobStatus(jobId);
            this.isComplete = (jobStatus.State == "Success" || jobStatus.State == "Failure");
            this.isRunning = (jobStatus.State == "Running");
            this.isSuccess = (jobStatus.State == "Success");

            if (this.isComplete) {

                this.pctRemaining = 0;

                if (this.isSuccess) {
                    this.pctComplete = 100;
                    this.pctFailed = 0;
                } else {
                    this.pctComplete = 0;
                    this.pctFailed = 100;
                }
            } else {
                this.pctFailed = 0;
                this.pctComplete = jobStatus.Progress;
                this.pctRemaining = 100 - this.pctComplete;
            }

            if (["DicomModalityStore", "OrthancPeerStore", "ResourceModification", "PushTransfer", "DicomWebStowClient"].indexOf(jobStatus.Type) != -1) {
                if ("Resources" in jobStatus.Content && this.showDetails && Object.keys(this.resources).length == 0) { // only update once !
                    if (jobStatus.Type == "DicomWebStowClient") {
                        if ("Studies" in jobStatus.Content.Resources) {
                            for (let rid of jobStatus.Content.Resources.Studies) {
                                let rj = {};
                                const study = await api.getStudy(rid);
                                rj["ID"] = rid;
                                rj["Type"] = "Study";
                                rj["LabelPrefix"] = this.$t("study") + ": ";
                                rj["LabelLink"] = this.getStudyLine(rid, study.MainDicomTags, study.PatientMainDicomTags);
                                rj["StudyInstanceUID"] = study.MainDicomTags.StudyInstanceUID;
                                this.resources[rj.ID] = rj;
                            } 
                        } 
                        if ("Series" in jobStatus.Content.Resources) {
                            for (let rid of jobStatus.Content.Resources.Series) {
                                let rj = {};
                                const study = await api.getSeriesParentStudy(rid);
                                rj["ID"] = rid;
                                rj["Type"] = "Series";
                                rj["LabelPrefix"] = this.$t("series_from_study") + ": ";
                                rj["LabelLink"] = this.getStudyLine(rid, study.MainDicomTags, study.PatientMainDicomTags);
                                rj["StudyInstanceUID"] = study.MainDicomTags.StudyInstanceUID;
                                this.resources[rj.ID] = rj;
                            } 
                        } 
                        if ("Patients" in jobStatus.Content.Resources) {
                            for (let rid of jobStatus.Content.Resources.Patients) {
                                let rj = {};
                                const patient = await api.getPatient(rid);
                                rj["ID"] = rid;
                                rj["Type"] = "Patient";
                                rj["LabelPrefix"] = this.$t("patient") + ": ";
                                rj["LabelLink"] = patient.MainDicomTags.PatientID + " - " + patient.MainDicomTags.PatientName;
                                rj["PatientID"] = patient.MainDicomTags.PatientID;
                                this.resources[rj.ID] = rj;
                            } 
                        }
                        // if, at this point, the resources are empty, it means we are sending only instances
                        if (Object.keys(this.resources).length == 0 && "Instances" in jobStatus.Content.Resources) {
                            for (let rid of jobStatus.Content.Resources.Patients) {
                                let rj = {};
                                const study = await api.getInstanceParentStudy(r.ID);
                                rj["ID"] = rid;
                                rj["Type"] = "Instance";
                                rj["LabelPrefix"] = this.$t("instances_from_study") + ": ";
                                rj["LabelLink"] = this.getStudyLine(r.ID, study.MainDicomTags, study.PatientMainDicomTags);
                                rj["StudyInstanceUID"] = study.MainDicomTags.StudyInstanceUID;
                                this.resources[rj.ID] = rj;
                            } 
                        }
                    } else {
                        for (let r of jobStatus.Content.Resources) {
                            
                            let type = r.Type;
                            if (jobStatus.Type == "PushTransfer") {
                                type = r.Level;
                            }
                            let rj = {};
                            
                            rj["ID"] = r.ID;
                            rj["Type"] = type;

                            if (type == "Study") {
                                const study = await api.getStudy(r.ID);
                                rj["LabelPrefix"] = this.$t("study") + ": ";
                                rj["LabelLink"] = this.getStudyLine(r.ID, study.MainDicomTags, study.PatientMainDicomTags);
                                rj["StudyInstanceUID"] = study.MainDicomTags.StudyInstanceUID;
                            } else if (type == "Series") {
                                const study = await api.getSeriesParentStudy(r.ID);
                                rj["LabelPrefix"] = this.$t("series_from_study") + ": ";
                                rj["LabelLink"] = this.getStudyLine(r.ID, study.MainDicomTags, study.PatientMainDicomTags);
                                rj["StudyInstanceUID"] = study.MainDicomTags.StudyInstanceUID;
                            } else if (type == "Instance") {
                                const study = await api.getInstanceParentStudy(r.ID);
                                rj["LabelPrefix"] = this.$t("instances_from_study") + ": ";
                                rj["LabelLink"] = this.getStudyLine(r.ID, study.MainDicomTags, study.PatientMainDicomTags);
                                rj["StudyInstanceUID"] = study.MainDicomTags.StudyInstanceUID;
                            } else if (type == "Patient") {
                                const patient = await api.getPatient(r.ID);
                                rj["LabelPrefix"] = this.$t("patient") + ": ";
                                rj["LabelLink"] = patient.MainDicomTags.PatientID + " - " + patient.MainDicomTags.PatientName;
                                rj["PatientID"] = patient.MainDicomTags.PatientID;
                            }

                            this.resources[rj.ID] = rj;
                        }
                    }
                    this.jobDetailsTitle = "resources";
                }
            }

            if (!this.isComplete) {
                this.refreshTimeout = Math.min(this.refreshTimeout + 200, 2000);  // refresh quickly at the beginnning !
                setTimeout(this.refresh, this.refreshTimeout, [jobId]);
            }
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
    async mounted() {
        this.refresh(this.job['id']);
    },
    components: { Modal }
}
</script>

<template>
    <div class="card border-secondary job-card">
        <div class="card-header jobs-header">
            {{ job.name }}
            <button type="button" class="btn-close job-card-close" aria-label="Close" @click="close(job.id)"></button>
            <div class="progress mt-1 mb-1" style="width:90%">
                <div class="progress-bar bg-success" role="progressbar"
                    v-bind:style="'width: ' + this.pctComplete + '%'">{{ statusText }}</div>
                <div class="progress-bar bg-secondary" role="progressbar"
                    v-bind:style="'width: ' + this.pctRemaining + '%'"></div>
                <div class="progress-bar bg-danger" role="progressbar" v-bind:style="'width: ' + this.pctFailed + '%'">
                    {{ statusText }}</div>
            </div>

        </div>
        <div class="card-body text-secondary jobs-body">
            <p v-if="shouldShowDetails" class="card-text">
                <span v-if="jobDetailsTitle">{{ jobDetailsTitle }}:
                <br /></span>
                <span v-for="(resource, resourceId) in resources" :key="resourceId">
                    {{  resource.LabelPrefix }}
                    <router-link v-if="resource.StudyInstanceUID"
                        v-bind:to="'/filtered-studies?StudyInstanceUID=' + resource.StudyInstanceUID + '&expand=study'">
                        {{ resource.LabelLink
                        }}</router-link>
                    <router-link v-if="resource.PatientID"
                        v-bind:to="'/filtered-studies?PatientID=' + resource.PatientID">
                        {{ resource.LabelLink
                        }}</router-link>
                    <br />
                </span>
            </p>
        </div>
    </div>
</template>
<style scoped></style>