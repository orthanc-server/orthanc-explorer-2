<script>

import { mapState } from "vuex"
import api from "../orthancApi"
import LabelHelpers from "../helpers/label-helpers"


export default {
    props: [],
    emits: [],
    data() {
        return {
            availableLabels: [],
            originalAvailableLabelsFromConfig: [],
            allUsedLabels: [],
            allOriginalLabels: [],
            newLabel: "",
            limitAvailableLabels: false,
            originalLimitAvailableLabels: false
        };
    },
    async mounted() {
        await this.reset();
    },
    methods: {
        async reset() {
            let config = await api.getRolesConfig();
            this.newLabel = "";
            this.originalAvailableLabelsFromConfig = config["available-labels"];
            this.allUsedLabels = await api.loadAllLabels();

            if (this.originalAvailableLabelsFromConfig.length == 0) {
                console.log("The available-labels list in the configuration is empty, getting the list of all current labels");
                this.limitAvailableLabels = false;
                this.allOriginalLabels = this.allUsedLabels;
                this.availableLabels = [...this.allUsedLabels];
            } else {
                this.limitAvailableLabels = true;
                this.availableLabels = [...this.originalAvailableLabelsFromConfig];
                this.allOriginalLabels = [...this.originalAvailableLabelsFromConfig];
            }
            this.originalLimitAvailableLabels = this.limitAvailableLabels;
        },
        createLabel() {
            if (this.availableLabels.indexOf(this.newLabel) == -1) { // ignore it if the label is already present in the list
                this.availableLabels.push(this.newLabel);
                this.availableLabels = this.availableLabels.sort();
            }
            this.newLabel = "";
        },
        deleteLabel(evetn, label) {
            let index = this.availableLabels.indexOf(label);
            if (index != -1) {
                this.availableLabels.splice(index, 1);
            }
            // console.log(this.availableLabels);
            // console.log(this.allOriginalLabels);
        },
        async save() {
            // reload the roles in case they have changed since we've loaded them and update the available-labels
            let config = await api.getRolesConfig();
            if (this.limitAvailableLabels) {
                config["available-labels"] = this.availableLabels;
            } else {
                config["available-labels"] = [];
            }
            await api.setRolesConfig(config);
            await this.$store.dispatch('configuration/load'); // to reload the available-labels
            this.reset();
        },
        cancel() {
            this.reset();
        },
        labelKeyPressed(event) {
            // console.log(event);
            let currentLabel = event.target.value;
            this.newLabel = LabelHelpers.filterLabel(currentLabel);
            // console.log(this.newLabel);
            if (event.keyCode == 13) { // Enter = createLabel
                this.createLabel();
            }
        },
        canDeleteLabel(label) {
            // we can delete a label only if it is currently not used
            return this.allUsedLabels.indexOf(label) == -1;
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        hasChanged() {
            return (this.originalLimitAvailableLabels != this.limitAvailableLabels)
                || (this.availableLabels.length != this.allOriginalLabels.length)
                || !this.availableLabels.every((v, i) => v === this.allOriginalLabels[i]);
        },
    }
}
</script>
<template>
    <div class="settings">
        <div>
            <h5>{{ $t('settings.available_labels_title') }}</h5>
            <div class="w-100 p-2 mx-0">
                <div class="form-check mt-3">
                    <input class="form-check-input" type="checkbox" value="" id="limit_labels"
                        v-model="limitAvailableLabels">
                    <label class="form-check-label" for="limit_labels">
                        {{ $t('settings.labels_limit_available_labels') }}
                    </label>
                </div>
                <div class="mt-1">
                    <p class="instructions-text" v-html="$t('settings.available_labels_global_instructions_html')"
                        :class="{ 'instructions-text-disabled': !limitAvailableLabels }"></p>
                </div>
                <div class="form-check" style="height: 3rem"><label for="new-label-input" class="form-label"><span
                            class="invalid-label-tips invalid-label-tips-hidden">{{ $t('labels.valid_alphabet_warning')
                            }}</span>
                    </label></div>
                <div class="form-check d-flex">
                    <input id="new-label-input" type="text" class="form-control w-50" v-model="newLabel"
                        @keyup="labelKeyPressed($event)" :disabled="!limitAvailableLabels">
                    <button type="button" class="btn btn-sm btn-secondary m-1" @click="createLabel"
                        :disabled="!limitAvailableLabels">{{
                            $t('settings.create_new_label') }}</button>
                </div>
                <div class="labels-list mt-5">
                    <div v-for="label in availableLabels" :key="label"
                        class="labels-item border d-flex justify-content-between align-items-center">
                        <span class="list-group-item-with-button-content"
                            :class="{ 'disabled-label': !limitAvailableLabels }">{{ label }}</span>
                        <button type="button" class="btn labels-button" @click="deleteLabel($event, label)"
                            :disabled="!canDeleteLabel(label) || !limitAvailableLabels"><i
                                class="bi-trash"></i></button>
                    </div>
                </div>
            </div>
            <div class="d-flex justify-content-end">
                <button type="button" class="btn btn-primary mx-1" :disabled="!hasChanged" @click="save"> {{ $t('save')
                }}</button>
                <button type="button" class="btn btn-secondary mx-1" :disabled="!hasChanged" @click="cancel">{{
                    $t('cancel') }}</button>
            </div>
        </div>
    </div>
</template>
<style scoped>
h5 {
    margin-top: 1rem;
    margin-bottom: 15px;
    margin-left: 50px;
}

h6 {
    font-weight: 600;
}

.settings {
    text-align: left;
}

.labels-list {
    display: flex;
    flex-flow: row wrap;
    margin: 2px;
    overflow: overlay;
}

.labels-item {
    flex: 0 1 auto;
    min-width: 15vw;
    height: 3rem;
    padding-left: 0.5rem;
    padding-bottom: 0;
    padding-right: 0.2rem;
    padding-top: 0;
    margin: 3px;
}

.disabled-label {
    opacity: 0.4;
}

.w-15 {
    width: 15%;
}

.instructions {
    margin-left: 1rem;
}

.instructions-text {
    font-size: smaller;
}

.instructions-text-disabled {
    opacity: 0.4;
}

.list-group-item-with-button-content {
    margin-top: var(--bs-list-group-item-padding-y);
    margin-bottom: var(--bs-list-group-item-padding-y);
}

.list-group-item-with-button {
    padding-left: var(--bs-list-group-item-padding-x);
    padding-bottom: 0;
    padding-right: var(--bs-list-group-item-padding-x);
    padding-top: 0;
}

.labels-button {
    /* background-color: #0002; */
    border-width: 0;
    padding-top: 0;
    padding-bottom: 0;
    font-size: larger;
}

.labels-button:disabled {
    opacity: 0.2;
}
</style>