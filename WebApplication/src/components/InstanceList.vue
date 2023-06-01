<script>
import InstanceItem from "./InstanceItem.vue"
import api from "../orthancApi"

export default {
    props: ['seriesId', 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags'],
    emits: ['deletedInstance'],
    data() {
        return {
            loaded: false,
            instancesInfo: {}
        };
    },
    computed: {
        sortedInstancesIds() {
            if (this.loaded) {
                let keys = Object.keys(this.instancesInfo);
                keys.sort((a, b) => (parseInt(this.instancesInfo[a].IndexInSeries) > parseInt(this.instancesInfo[b].IndexInSeries) ? 1 : -1))
                return keys;
            } else {
                return [];
            }
        }
    },
    async mounted() {
        const response = await api.getSeriesInstances(this.seriesId)
        for (const instanceInfo of response) {
            this.instancesInfo[instanceInfo.ID] = instanceInfo;
        }
        this.loaded = true;
    },
    methods: {
        onDeletedInstance(instanceId) {
            delete this.instancesInfo[instanceId];
            this.$emit("deletedInstance", instanceId);
        }
    },
    components: { InstanceItem }
}
</script>

<template>
    <table class="table table-responsive table-sm instance-table">
        <thead>
            <th width="2%" scope="col" class="instance-table-header"></th>
            <th width="7%" scope="col" class="instance-table-header cut-text" data-bs-toggle="tooltip"
                :title="$t('dicom_tags.InstanceNumber')">{{ $t('dicom_tags.InstanceNumber') }}</th>
            <th width="40%" scope="col" class="instance-table-header cut-text" data-bs-toggle="tooltip"
                title="SOP Instance UID">SOP Instance UID</th>
            <th width="5%" scope="col" class="series-table-header cut-text text-center" data-bs-toggle="tooltip"
                :title="$t('dicom_tags.NumberOfFrames')"># {{$t('frames')}}</th>
        </thead>
        <InstanceItem v-for="instanceId in sortedInstancesIds" :key="instanceId" :instanceId="instanceId"
            :instanceInfo="instancesInfo[instanceId]" :studyMainDicomTags="this.studyMainDicomTags"
            :seriesMainDicomTags="this.seriesMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags"
            @deletedInstance="onDeletedInstance"></InstanceItem>
    </table>
</template>

<style>
.instance-table> :not(:first-child) {
    border-top: 0px !important;
}

.instance-table>:first-child {
    border-bottom: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}

.instance-table>:nth-child(odd) {
    background-color: var(--instance-odd-color);
}

.instance-table-header {
    text-align: left;
    padding-left: 10px;
}

.instance-table td {
    text-align: left;
    padding-left: 10px;
}
</style>