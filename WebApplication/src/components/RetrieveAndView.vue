<script>
import { mapState } from "vuex"
import api from "../orthancApi"


export default {
    props: [],
    async created() {
        const params = new URLSearchParams(window.location.search);
        
        if (params.has("StudyInstanceUID")) {
            this.studyInstanceUid = params.get('StudyInstanceUID');
        } else if (params.has("study")) {
            this.studyInstanceUid = params.get('study');
        } else {
            console.error("No study defined.  Use StudyInstanceUID=1.2.3.... in your url");
        }
        
        const modality = params.get('modality');
        if (params.has("viewer")) {
            this.viewer = params.get("viewer");
        }

        const study = await api.findStudy( this.studyInstanceUid);
        if (study == null) {
            this.state = "finding-remotely";

            const remoteStudies = await api.remoteDicomFindStudies(modality, {"StudyInstanceUID": this.studyInstanceUid});
            if (remoteStudies.length == 0) {
                this.state = "not-found";
            } else {
                if (this.remoteMode == 'dicom') {
                    const moveQuery = {
                        "StudyInstanceUID": this.studyInstanceUid
                    };

                    const jobId = await api.remoteDicomRetrieveStudy(modality, moveQuery, this.system.DicomAet, "Study");
                    this.state = "retrieving";
                    this.startMonitoringJob(jobId);
                }                
            }
        } else {
            this.studyOrthancId = study['ID'];
            this.openViewer();
        }
    },
    setup() {
        return {
        }
    },

    data() {
        return {
            state: "finding-locally",
            remoteMode: "dicom",
            viewer: "stone-viewer",
            retrievedInstancesCount: 0,
            studyInstanceUid: null,
            studyOrthancId: null,
            jobProgressComplete: 0,
            jobProgressFailed: 0,
            jobProgressRemaining: 100,
            jobRefreshTimeout: 200,
            jobIsComplete: false,
            jobIsRunning: false,
            jobIsSuccess: false
        };
    },
    mounted() {
    },
    methods: {
        startMonitoringJob(jobId) {
            this.jobRefreshTimeout = 200;  // refresh quickly at the beginnning !
            setTimeout(this.monitorJob, this.jobRefreshTimeout, [jobId]);
        },
        openViewer() {
            if (this.viewer == "stone-viewer") {
                window.location.href = api.getStoneViewerUrl("study", this.studyInstanceUid);
            } else if (this.viewer == "osimis-viewer") {
                window.location.href = api.getOsimisViewerUrl("study", this.studyOrthancId);
            } else if (this.viewer == "ohif-viewer") {
                window.location.href = api.getOh("study", this.studyOrthancId);
            } else {
                console.error("unsupported viewer: ", this.viewer);
            }
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
                if (this.studyOrthancId == null) {
                    const study = await api.findStudy(this.studyInstanceUid);
                    if (study != null) {
                        this.studyOrthancId = study['ID'];
                    }
                } else {
                    const instances = await api.getStudyInstances(this.studyOrthancId);
                    this.retrievedInstancesCount = instances.length;
                }

                this.jobProgressFailed = 0;
                this.jobProgressComplete = jobStatus.Progress;
                this.jobProgressRemaining = 100 - this.jobProgressComplete;
            }
            if (!this.jobIsComplete) {
                this.jobRefreshTimeout = Math.min(this.jobRefreshTimeout + 200, 2000);  // refresh quickly at the beginnning !
                setTimeout(this.monitorJob, this.jobRefreshTimeout, [jobId]);
            } else {
                this.openViewer();
            }
        },
    },
    computed: {
        ...mapState({
            system: state => state.configuration.system,
        })
    },
    components: {}
}
</script>

<template>
    <div class="d-flex flex-column min-vh-100 justify-content-center align-items-center h4 text-center">
        <span>
            <p  v-if="state=='finding-locally'" v-html="$t('retrieve_and_view.finding_locally')"></p>
            <p  v-if="state=='finding-remotely'" v-html="$t('retrieve_and_view.finding_remotely')"></p>
            <p  v-if="state=='not-found'" v-html="$t('retrieve_and_view.not_found')"></p>
            <p  v-if="state=='retrieving'" v-html="$t('retrieve_and_view.retrieving')"></p>
            <p  v-if="state=='retrieving'" v-html="$t('retrieve_and_view.retrieved_html', { count: retrievedInstancesCount })"></p>
            <div class="spinner-border" role="status">
                <span class="visually-hidden">Loading...</span>
            </div>
        </span>
    </div>
</template>

<style></style>