<script>
import ResourceButtonGroup from "./ResourceButtonGroup.vue";
import InstanceList from "./InstanceList.vue";

export default {
    props: ['seriesId', 'seriesMainDicomTags', 'instancesIds'],
    emits: ['deletedSeries'],
    setup() {
    },
    data() {
        return {
        };
    },
    computed: {
    },
    components: { ResourceButtonGroup, InstanceList },
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
        <tr>
            <td width="70%" class="cut-text">
                <ul>
                    <li class="d-flex">
                        <span class="details-label">Series Date:</span>
                        <span class="details">{{ seriesMainDicomTags.SeriesDate }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Series Time:</span>
                        <span class="details">{{ seriesMainDicomTags.SeriesTime }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Series Description:</span>
                        <span class="details">{{ seriesMainDicomTags.SeriesDescription }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Series Number:</span>
                        <span class="details">{{ seriesMainDicomTags.SeriesNumber }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Body Part Examined:</span>
                        <span class="details">{{ seriesMainDicomTags.BodyPartExamined }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label">Protocol Name:</span>
                        <span class="details">{{ seriesMainDicomTags.ProtocolName }}</span>
                    </li>
                    <li class="d-flex">
                        <span class="details-label d-inline-block text-truncate">Series Instance UID:</span>
                        <span class="details">{{ seriesMainDicomTags.SeriesInstanceUID }}</span>
                    </li>
                </ul>
            </td>
            <td width="30%" class="series-button-group">
                <ResourceButtonGroup
                :resourceOrthancId="this.seriesId"
                :resourceLevel="'series'"
                :resourceDicomUid="this.seriesMainDicomTags.SeriesInstanceUID"
                :customClass="'instance-button-group'"
                @deletedResource="onDeletedSeries"
                ></ResourceButtonGroup>
            </td>
        </tr>
        <tr>
            <td colspan="100">
                <InstanceList
                    :seriesId="this.seriesId"
                    :seriesMainDicomTags="this.seriesMainDicomTags"
                    :instancesIds="this.instancesIds"
                    @deletedInstance="onDeletedInstance"
                ></InstanceList>
            </td>
        </tr>
    </table>
</template>

<style scoped>
.series-details-table {
    margin-top: var(--details-top-margin);
    margin-left: 5%;
    width: 95% !important;
    background-color: var(--series-selected-color);
    font-size: 0.8rem;
}

.series-details-table td {
    vertical-align: top;
}

.details-label {
    font-weight: 700;
    max-width: 30%;
    overflow: hidden;
    text-overflow: ellipsis;
}

.details {
    margin-left: auto !important;
    font-weight: 500;
    max-width: 25vw;
    overflow: hidden;
    text-overflow: ellipsis;
}

</style>
<style>
.series-button-group i {
    font-size: 1.3rem;
}
</style>
