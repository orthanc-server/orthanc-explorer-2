<script>
import ResourceDetailText from "./ResourceDetailText.vue";
import { mapState, mapGetters } from "vuex"

import api from "../orthancApi"


export default {
    props: ['studyFields', 'id', 'remoteMode', 'remoteSource'], // studyFields = the "answer" from Orthanc to /modalities/.../queries -> /queries/.../answers?expand
    setup() {
    },
    data() {
        return {
        };
    },
    computed: {
        ...mapState({
            system: state => state.configuration.system
        })
    },
    components: { ResourceDetailText },
    methods: {
        async retrieve() {
            if (this.remoteMode == 'dicom') {
                const moveQuery = {
                    "StudyInstanceUID": this.studyFields.StudyInstanceUID,
                    "PatientID": this.studyFields.PatientID
                };

                const jobId = await api.remoteDicomRetrieveStudy(this.remoteSource, moveQuery, this.system.DicomAet, "Study");
                this.$store.dispatch('jobs/addJob', { jobId: jobId, name: 'Retrieve Study from (' + this.remoteSource + ')'});
            }            
        }
    }

}
</script>


<template>
    <table class="table table-responsive table-sm study-details-table">
        <tr>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText :value="studyFields.StudyDate">Study Date</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.StudyTime">Study Time</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.StudyDescription" :truncate="true">Study Description</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.AccessionNumber">Accession Number</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.StudyID">Study ID</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.StudyInstanceUID" :truncate="true">Study Instance UID</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.RequestingPhysician">RequestingPhysician</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.ReferringPhysicianName">ReferringPhysicianName</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.InstitutionName">InstitutionName</ResourceDetailText>
                </ul>
            </td>
            <td width="40%" class="cut-text">
                <ul>
                    <ResourceDetailText :value="studyFields.PatientID">Patient ID</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.PatientName">Patient Name</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.PatientBirthDate">Patient Birth Date</ResourceDetailText>
                    <ResourceDetailText :value="studyFields.PatientSex">Patient Sex</ResourceDetailText>
                </ul>
            </td>
            <td width="20%" class="study-button-group">
                <div class="d-grid d-lg-block gap-2">
                    <a
                        class="btn btn-sm btn-secondary m-1"
                        type="button"
                        data-bs-toggle="tooltip"
                        title="Retrieve"
                        @click="retrieve()"
                    >
                        <i class="bi bi-box-arrow-in-down"></i>
                    </a>
                </div>
            </td>
        </tr>
        <!-- <tr>
            <td colspan="100">
                <SeriesList :studyId="this.studyId" @deletedStudy="onDeletedStudy"></SeriesList>
            </td>
        </tr> -->
    </table>
</template>

<style scoped>
.study-details-table {
    margin-top: var(--details-top-margin);
    margin-left: 5%;
    width: 95% !important;
    background-color: var(--study-selected-color);
    font-size: 0.9rem;
}

.study-details-table td {
    vertical-align: top;
}

</style>
<style>
.study-button-group i {
    font-size: 1.4rem;
}
</style>
