<script>

import { mapState } from "vuex"
import api from "../orthancApi"
import LabelHelpers from "../helpers/label-helpers"


export default {
    props: [],
    emits: [],
    data() {
        return {
            allLabels: [],
            allOriginalLabels: [],
            newLabel: ""
        };
    },
    async mounted() {
        await this.reset();
    },
    methods: {
        async reset() {
            let config = await api.getRolesConfig();
            this.newLabel = "";
            this.allOriginalLabels = config["allowed-labels"];
            if (this.allOriginalLabels.length == 0) {
                console.log("The allowed-labels list is empty, getting the list of all current labels");
                this.allOriginalLabels = await api.loadAllLabels();
            }
            this.allLabels = [...this.allOriginalLabels];
        },
        createLabel() {
            if (this.allLabels.indexOf(this.newLabel) == -1) { // ignore it if the label is already present in the list
                this.allLabels.push(this.newLabel);
                this.allLabels = this.allLabels.sort();
            }
            this.newLabel = "";
        },
        deleteLabel(evetn, label) {
            let index = this.allLabels.indexOf(label);
            if (index != -1) {
                this.allLabels.splice(index, 1);
            }
            console.log(this.allLabels);
            console.log(this.allOriginalLabels);
        },
        async save() {
            // reload the roles in case they have changed since we've loaded them and update the allowed-labels
            let config = await api.getRolesConfig();
            config["allowed-labels"] = this.allLabels;
            await api.setRolesConfig(config);
            this.reset();
        },
        cancel() {
            this.reset();
        },
        labelKeyPressed(event) {
            // console.log(event);
            let currentLabel = event.target.value;
            this.newLabel = LabelHelpers.filterLabel(currentLabel);
            console.log(this.newLabel);
            if (event.keyCode == 13) { // Enter = createLabel
                this.createLabel();
            }
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        labels() {
            return this.allLabels;
        },
        hasChanged() {
            return (this.allLabels.length != this.allOriginalLabels.length)
                || !this.allLabels.every((v, i) => v === this.allOriginalLabels[i]);
        },
    },
}
</script>
<template>
    <div class="settings">
        <div>
            <h5>{{ $t('settings.labels_title') }}</h5>
            <p class="instructions" v-html="$t('settings.labels_global_instructions_html')"></p>
            <div class="w-100 p-2 mx-0">
                <div style="height: 3rem"><label for="new-label-input" class="form-label"><span
                            class="invalid-label-tips invalid-label-tips-hidden">{{ $t('labels.valid_alphabet_warning')
                            }}</span>
                    </label></div>
                <div class="form-check d-flex">
                    
                    <input id="new-label-input" type="text" class="form-control w-50" v-model="newLabel" @keyup="labelKeyPressed($event)">
                    <button type="button" class="btn btn-sm btn-secondary m-1" @click="createLabel">{{
                $t('settings.create_new_label') }}</button>
                </div>
                <div class="labels-list mt-5">
                    <div v-for="label in labels" :key="label"
                        class="labels-item border d-flex justify-content-between align-items-center">
                        <span class="list-group-item-with-button-content">{{ label }}</span>
                        <button type="button" class="btn labels-button" @click="deleteLabel($event, label)"><i
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
    margin-top: 50px;
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
    flex-flow: column wrap;
    height: 60vh;
    margin: 2px;
    overflow: overlay;
}

.labels-item {
    min-width: 15vw;
    min-height: 3rem;
    padding-left: 0.5rem;
    padding-bottom: 0;
    padding-right: 0.2rem;
    padding-top: 0;
    margin: 3px;
}

.w-15 {
    width: 15%;
}

.w-10 {
    width: 15%;
}

.disabled-plugin {
    text-decoration: line-through;
}

.header {
    padding-left: 15px;
}

.value {
    text-align: right;
    padding-right: 15px;
}

.instructions {
    margin-left: 1rem;
}

.instructions-text {
    font-size: smaller;
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
    border-radius: 0;
    padding-top: 0;
    padding-bottom: 0;
    font-size: larger;
}
</style>