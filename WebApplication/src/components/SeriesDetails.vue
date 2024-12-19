<script>
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import InstanceList from "./InstanceList.vue";
import InstanceListExtended from "./InstanceListExtended.vue";
import ResourceDetailText from "./ResourceDetailText.vue";
import { mapState, mapGetters } from "vuex"
import api from "../orthancApi";
import SourceType from '../helpers/source-type';


export default {
    props: ['seriesId', 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags', 'instancesIds'],
    emits: ['deletedSeries'],
    setup() {
    },
    data() {
        return {
            seriesInstances: []
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            studiesSourceType: state => state.studies.sourceType,
            studiesRemoteSource: state => state.studies.remoteSource,
            hasExtendedFind: state => state.configuration.hasExtendedFind
        }),
        useExtendedInstanceList() {
            return this.hasExtendedFind && this.studiesSourceType == SourceType.LOCAL_ORTHANC;
        }
    },
    async mounted() {
        if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
            if (this.useExtendedInstanceList) {
                this.seriesInstances = await api.getSeriesInstancesExtended(this.seriesId, null);
            } else {
                this.seriesInstances = await api.getSeriesInstances(this.seriesId);
            }
        } else if (this.studiesSourceType == SourceType.REMOTE_DICOM) {
            let remoteInstances = (await api.remoteDicomFind("Instance", this.studiesRemoteSource, {
                    "StudyInstanceUID": this.studyMainDicomTags.StudyInstanceUID,
                    "PatientID": this.patientMainDicomTags.PatientID,
                    "SeriesInstanceUID": this.seriesMainDicomTags.SeriesInstanceUID,
                    "SOPInstanceUID": "",
                    "InstanceNumber": "",
                    "NumberOfFrames": ""
                },
                false /* isUnique */));
            this.seriesInstances = remoteInstances.map(s => { return {
                "ID": s["SOPInstanceUID"],
                "MainDicomTags": s
            }})
            this.seriesInstances = this.seriesInstances.sort((a, b) => (parseInt(a.MainDicomTags.InstanceNumber) ?? a.MainDicomTags.SOPInstanceUID) < (parseInt(b.MainDicomTags.InstanceNumber) ?? b.MainDicomTags.SOPInstanceUID) ? 1 : -1);
        } else if (this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
            let remoteInstances = (await api.qidoRs("Instance", this.studiesRemoteSource, {
                    "StudyInstanceUID": this.studyMainDicomTags.StudyInstanceUID,
                    "SeriesInstanceUID": this.seriesMainDicomTags.SeriesInstanceUID,
                    "SOPInstanceUID": "",
                    "InstanceNumber": "",
                    "NumberOfFrames": ""
                },
                false /* isUnique */));
            this.seriesInstances = remoteInstances.map(s => { return {
                "ID": s["SOPInstanceUID"],
                "MainDicomTags": s
            }})
            this.seriesInstances = this.seriesInstances.sort((a, b) => (parseInt(a.MainDicomTags.InstanceNumber) ?? a.MainDicomTags.SOPInstanceUID) < (parseInt(b.MainDicomTags.InstanceNumber) ?? b.MainDicomTags.SOPInstanceUID) ? 1 : -1);
        }
    },
    components: { ResourceButtonGroup, InstanceList, InstanceListExtended, ResourceDetailText },
    methods: {
        onDeletedInstance(instanceId) {
            const pos = this.instancesIds.indexOf(instanceId);
            if (pos >= 0) {
                this.instancesIds.splice(pos, 1);
                if (this.instancesIds.length == 0) {
                    this.$emit("deletedSeries", this.seriesId);
                }
            }
        },
        onDeletedSeries() {
            this.$emit("deletedSeries", this.seriesId);
        }
    }

}
</script>


<template>
    <table class="table table-responsive table-sm series-details-table">
        <tbody>
            <tr>
                <td width="70%" class="cut-text">
                    <ul>
                        <ResourceDetailText v-for="tag in uiOptions.SeriesMainTags" :key="tag" :tags="seriesMainDicomTags" :tag="tag" :showIfEmpty="true"></ResourceDetailText>
                    </ul>
                </td>
                <td width="30%" class="series-button-group">
                    <ResourceButtonGroup
                    :resourceOrthancId="this.seriesId"
                    :resourceLevel="'series'"
                    :resourceDicomUid="this.seriesMainDicomTags.SeriesInstanceUID"
                    :studyMainDicomTags="this.studyMainDicomTags"
                    :seriesMainDicomTags="this.seriesMainDicomTags"
                    :patientMainDicomTags="this.patientMainDicomTags"
                    :seriesInstances="this.seriesInstances"
                    :customClass="'instance-button-group'"
                    @deletedResource="onDeletedSeries"
                    ></ResourceButtonGroup>
                </td>
            </tr>
            <tr>
                <td colspan="100">
                    <InstanceList v-if="!useExtendedInstanceList"
                        :seriesId="this.seriesId"
                        :seriesMainDicomTags="this.seriesMainDicomTags"
                        :patientMainDicomTags="this.patientMainDicomTags"
                        :studyMainDicomTags="this.studyMainDicomTags"
                        :instancesIds="this.instancesIds"
                        :seriesInstances="this.seriesInstances"
                        @deletedInstance="onDeletedInstance"
                    ></InstanceList>
                    <InstanceListExtended v-if="useExtendedInstanceList"
                        :seriesId="this.seriesId"
                        :seriesMainDicomTags="this.seriesMainDicomTags"
                        :patientMainDicomTags="this.patientMainDicomTags"
                        :studyMainDicomTags="this.studyMainDicomTags"
                        :instancesIds="this.instancesIds"
                        :seriesInstances="this.seriesInstances"
                        @deletedInstance="onDeletedInstance"
                    ></InstanceListExtended>
                </td>
            </tr>
        </tbody>
    </table>
</template>

<style scoped>
.series-details-table {
    margin-top: var(--details-top-margin);
    margin-left: 5%;
    width: 95% !important;
    background-color: var(--series-details-bg-color);
    font-size: 0.8rem;
}

.series-details-table td {
    vertical-align: top;
}

.series-details-table>:not(caption) >* >* {
  background-color: var(--series-details-bg-color) !important;
}

.series-details-table >* >* {
  background-color: var(--series-details-bg-color) !important;
}

</style>
<style>
.series-button-group i {
    font-size: 1.3rem;
}
</style>