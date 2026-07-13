<script>
import InstanceItem from "./InstanceItem.vue"
import api from "../orthancApi"
import { ObserveVisibility as vObserveVisibility } from 'vue3-observe-visibility'
import SelectionStatus from "../helpers/selection-status";


export default {
    props: ['seriesId', 'seriesMainDicomTags', 'studyMainDicomTags', 'patientMainDicomTags', 'seriesInstances', 'studyId', 'studySeries'],
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
        selectedInstancesCount() {
            const count = this.$store.getters['selection/getSelectedInstancesCount'](this.studyId, this.seriesId, this.seriesInstances);
            if (count == 0) {
                return "";
            } else {
                return count;
            }
        },
        allSelected() {
            return this.$store.getters['selection/getSeriesSelectionStatus'](this.studyId, this.seriesId) != SelectionStatus.NOT_SELECTED;
        },
        isPartialySelected() {
            return this.$store.getters['selection/getSeriesSelectionStatus'](this.studyId, this.seriesId) == SelectionStatus.PARTIAL;
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

        },
        clickSelectAll() {
            if (this.allSelected == '' || !this.allSelected) { // this is the value before the click
                this.$store.dispatch('selection/selectSeries', { studyId: this.studyId, seriesId: this.seriesId, studySeries: this.studySeries, isSelected: true });
            } else {
                this.$store.dispatch('selection/selectSeries', { studyId: this.studyId, seriesId: this.seriesId, studySeries: this.studySeries, isSelected: false });
            }
        },
    },
    components: { InstanceItem }
}
</script>

<template>
    <table class="table table-responsive table-sm instance-table">
        <thead>
            <tr>
                <th width="2%" scope="col" class="instance-table-header">
                    <div class="form-check" style="margin-left: 0.5rem">
                        <input class="form-check-input" type="checkbox" :checked="allSelected"
                            :indeterminate="isPartialySelected" @click="clickSelectAll"><span
                            style="font-weight: 400; font-size: small;">{{ selectedInstancesCount
                            }}</span>
                    </div>
                </th>
                <th width="7%" scope="col" class="instance-table-header cut-text" data-bs-toggle="tooltip"
                    :title="$t('dicom_tags.InstanceNumber')">{{ $t('dicom_tags.InstanceNumber') }}</th>
                <th width="40%" scope="col" class="instance-table-header cut-text" data-bs-toggle="tooltip"
                    title="SOP Instance UID">SOP Instance UID</th>
                <th width="5%" scope="col" class="series-table-header cut-text text-center" data-bs-toggle="tooltip"
                    :title="$t('dicom_tags.NumberOfFrames')"># {{ $t('frames') }}</th>
            </tr>
        </thead>
        <InstanceItem v-for="instanceId in sortedInstancesIds" :key="instanceId" :id="instanceId"
            :instanceId="instanceId" :instanceInfo="instancesInfo[instanceId]"
            :studyMainDicomTags="this.studyMainDicomTags" :seriesMainDicomTags="this.seriesMainDicomTags"
            :patientMainDicomTags="this.patientMainDicomTags" @deletedInstance="onDeletedInstance"
            :studyId="this.studyId" :studySeries="this.studySeries" :seriesInstances="this.seriesInstances"
            :seriesId="this.seriesId"
            v-observe-visibility="{ callback: visibilityChanged, once: true }">
        </InstanceItem>
    </table>
</template>

<style>
.instance-table {}

.instance-table> :not(:first-child) {
    border-top: 0px !important;
}

.instance-table>:first-child {
    border-bottom: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}

.instance-table>:nth-child(odd)>*>* {
    background-color: var(--instance-odd-bg-color);
}

.instance-table>:nth-child(even)>*>* {
    background-color: var(--instance-even-bg-color);
}

.instance-table td {
    text-align: left;
    padding-left: 10px;
}

.instance-table>tbody>tr:hover>* {
    background-color: var(--instance-hover-color);
}

.instance-table>tbody>tr.instance-row-expanded:hover>* {
    background-color: var(--instance-details-bg-color);
}

.instance-table>tbody>tr.instance-details-expanded:hover>* {
    background-color: var(--instance-details-bg-color);
}


.instance-table-header {
    text-align: left;
    padding-left: 10px;
}
</style>
