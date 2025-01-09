<script>
import InstanceItem from "./InstanceItem.vue"
import api from "../orthancApi"
import { ObserveVisibility as vObserveVisibility } from 'vue3-observe-visibility'


export default {
    props: ['seriesId', 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags', 'seriesInstances'],
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
        },
    },
    watch: {
        seriesInstances(newValue, oldValue) {
            for (const instanceInfo of this.seriesInstances) {
                this.instancesInfo[instanceInfo.ID] = instanceInfo;
            }
            this.loaded = true;
        }
    },
    async mounted() {
    },
    methods: {
        onDeletedInstance(instanceId) {
            delete this.instancesInfo[instanceId];
            this.$emit("deletedInstance", instanceId);
        },
        async visibilityChanged(isVisible, entry) {
            if (isVisible) {
                let instanceId = entry.target.id;
                // console.log(instanceId, this.seriesInstances);
                if (instanceId == this.seriesInstances[this.seriesInstances.length - 1]['ID']) {
                    // console.log("Last element shown -> should load more instances");
                    const instances = await api.getSeriesInstancesExtended(this.seriesId, this.seriesInstances.length);
                    this.seriesInstances.push(...instances);
                    for (const instanceInfo of instances) {
                        this.instancesInfo[instanceInfo.ID] = instanceInfo;
                    }

                }
            }
            
        }
    },
    components: { InstanceItem }
}
</script>

<template>
    <table class="table table-responsive table-sm instance-table">
        <thead>
            <tr>
                <th width="2%" scope="col" class="instance-table-header"></th>
                <th width="7%" scope="col" class="instance-table-header cut-text" data-bs-toggle="tooltip"
                    :title="$t('dicom_tags.InstanceNumber')">{{ $t('dicom_tags.InstanceNumber') }}</th>
                <th width="40%" scope="col" class="instance-table-header cut-text" data-bs-toggle="tooltip"
                    title="SOP Instance UID">SOP Instance UID</th>
                <th width="5%" scope="col" class="series-table-header cut-text text-center" data-bs-toggle="tooltip"
                    :title="$t('dicom_tags.NumberOfFrames')"># {{$t('frames')}}</th>
            </tr>
        </thead>
        <InstanceItem v-for="instanceId in sortedInstancesIds" :key="instanceId" :id="instanceId" :instanceId="instanceId"
            :instanceInfo="instancesInfo[instanceId]" :studyMainDicomTags="this.studyMainDicomTags"
            :seriesMainDicomTags="this.seriesMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags"
            @deletedInstance="onDeletedInstance" v-observe-visibility="{callback: visibilityChanged, once: true}">
        </InstanceItem>
    </table>
</template>

<style>
.instance-table {
}

.instance-table> :not(:first-child) {
    border-top: 0px !important;
}

.instance-table>:first-child {
    border-bottom: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}

.instance-table>:nth-child(odd) >* >* {
    background-color: var(--instance-odd-bg-color);
}

.instance-table>:nth-child(even) >* >* {
    background-color: var(--instance-even-bg-color);
}

.instance-table td {
    text-align: left;
    padding-left: 10px;
}

.instance-table > tbody > tr:hover > * {
    background-color: var(--instance-hover-color);
}

.instance-table > tbody > tr.instance-row-expanded:hover > * {
    background-color: var(--instance-details-bg-color);
}
.instance-table > tbody > tr.instance-details-expanded:hover > * {
    background-color: var(--instance-details-bg-color);
}


.instance-table-header {
    text-align: left;
    padding-left: 10px;
}

</style>
