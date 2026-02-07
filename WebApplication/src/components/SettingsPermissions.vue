<script>

import { mapState } from "vuex"
import api from "../orthancApi"
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import { nextTick } from "vue";


export default {
    props: [],
    emits: [],
    data() {
        return {
            allLabels: [],
            selectedLabels: [],
            allLabelPermissions: [],
            allGlobalPermissions: ["admin-permissions", "edit-labels", "q-r-remote-modalities", "settings", "upload"],
            selectedLabelPermissions: [],
            selectedGlobalPermissions: [],
            hasAllLabelsAllowed: false,
            currentConfig: {},
            originalConfig: {},
            selectedRole: null,
            hasChanged: false,     // used to enable/disable the Save button
            disableWatcher: false,
        };
    },
    async mounted() {
        var popoverTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="popover"]'))
        var popoverList = popoverTriggerList.map(function (popoverTriggerEl) {
            return new bootstrap.Popover(popoverTriggerEl)
        });
        this.reset();
    },
    watch: {
        hasAllLabelsAllowed(newValue, oldValue) {
            if (this.disableWatcher) {
                return;
            }

            // clear the selection anyway 
            // -> if newValue is now false -> start from an empty list
            // -> if newValue is now true -> updateHasChanged will set the selectedLabels to ['*']
            this.selectedLabels = [];
            this.updateHasChanged();
        },
        selectedRole(newValue, oldValue) {
            if (this.disableWatcher) {
                return;
            }

            this.updateHasChanged();
        },
        selectedLabels(newValue, oldValue) {
            if (this.disableWatcher) {
                return;
            }

            this.updateHasChanged();
        },
        selectedLabelPermissions(newValue, oldValue) {
            if (this.disableWatcher) {
                return;
            }

            this.updateHasChanged();
        }
    },
    methods: {
        async reset() {
            this.originalConfig = await api.getRolesConfig();
            this.currentConfig = JSON.parse(JSON.stringify(this.originalConfig));
            this.allLabelPermissions = this.filterLabelPermissions(await api.getPermissionsList(), false);
            this.allGlobalPermissions = this.filterGlobalPermissions(await api.getPermissionsList(), false);
            this.allLabels = this.currentConfig["available-labels"];

            if (this.allLabels.length == 0) {
                console.log("The available-labels list is empty, getting the list of all current labels");
                this.allLabels = await api.loadAllLabels();
            }

            if (this.selectedRole == null) {
                let firstRole = Object.keys(this.currentConfig["roles"])[0];
                this.selectRole(firstRole);
            } else {
                this.selectRole(this.selectedRole);
            }
            this.updateHasChanged();
        },
        filterLabelPermissions(permissionsList, expandAll) {
            if (expandAll && permissionsList.indexOf("all") != -1) {
                // replace "all" by the full list of label permissions
                return [...this.allLabelPermissions];
            } else {
                let filtered = permissionsList.filter((perm) => this.allGlobalPermissions.indexOf(perm) == -1 && perm != "all");
                return filtered;
            }
        },
        filterGlobalPermissions(permissionsList, expandAll) {
            if (expandAll && permissionsList.indexOf("all") != -1) {
                // replace "all" by the full list of global permissions 
                return [...this.allGlobalPermissions];
            } else {
                let filtered = permissionsList.filter((perm) => this.allGlobalPermissions.indexOf(perm) != -1);
                return filtered;
            }
        },
        getCurrentUiRoles() {
            // write the definition of the role we are leaving into the "json" structure
            if (this.selectedRole != null) {
                let config = JSON.parse(JSON.stringify(this.currentConfig)); // make a copy
                let permissions = [...this.selectedLabelPermissions].concat(this.selectedGlobalPermissions);
                config["roles"][this.selectedRole]["permissions"] = permissions;

                if (this.hasAllLabelsAllowed) {
                    config["roles"][this.selectedRole]["authorized-labels"] = ["*"];
                } else {
                    config["roles"][this.selectedRole]["authorized-labels"] = this.selectedLabels;
                }
                return config;
            } else {
                return this.currentConfig;
            }

        },
        async selectRole(role) {
            this.disableWatcher = true;

            if (this.selectedRole != null) {
                // write the definition of the role we are leaving into the "json" structure
                this.currentConfig = this.getCurrentUiRoles();
            }

            this.selectedRole = role;
            this.selectedLabelPermissions = this.filterLabelPermissions(this.currentConfig["roles"][this.selectedRole]["permissions"], true);
            this.selectedGlobalPermissions = this.filterGlobalPermissions(this.currentConfig["roles"][this.selectedRole]["permissions"], true);
            this.selectedLabels = this.currentConfig["roles"][this.selectedRole]["authorized-labels"];
            this.hasAllLabelsAllowed = this.currentConfig["roles"][this.selectedRole]["authorized-labels"].indexOf("*") != -1;

            await nextTick();
            this.disableWatcher = false;
        },
        isRoleSelected(role) {
            return this.selectedRole == role;
        },
        isLabelSelected(label) {
            return this.selectedLabels.indexOf(label) != -1;
        },
        toggleSelectedLabel(event, label) {
            event.preventDefault();
            let index = this.selectedLabels.indexOf(label);
            if (index != -1) {
                this.selectedLabels.splice(index, 1);
            } else {
                this.selectedLabels.push(label);
            }
            this.updateHasChanged();
        },
        selectAllLabels() {
            this.selectedLabels = [...this.allLabels];
        },
        clearSelectedLabels() {
            this.selectedLabels = [];
        },
        isPermissionSelected(perm, labelOrGlobal) {
            if (labelOrGlobal == "label") {
                return this.selectedLabelPermissions.indexOf(perm) != -1;
            } else {
                return this.selectedGlobalPermissions.indexOf(perm) != -1;
            }
        },
        toggleSelectedPermission(event, perm, labelOrGlobal) {
            event.preventDefault();
            if (labelOrGlobal == "label") {
                let index = this.selectedLabelPermissions.indexOf(perm);
                if (index != -1) {
                    this.selectedLabelPermissions.splice(index, 1);
                } else {
                    this.selectedLabelPermissions.push(perm);
                }
            } else {
                let index = this.selectedGlobalPermissions.indexOf(perm);
                if (index != -1) {
                    this.selectedGlobalPermissions.splice(index, 1);
                } else {
                    this.selectedGlobalPermissions.push(perm);
                }
            }
            this.updateHasChanged();
        },
        clearSelectedLabelPermissions() {
            this.selectedLabelPermissions = [];
        },
        selectAllLabelPermissions() {
            this.selectedLabelPermissions = [...this.allLabelPermissions];
        },
        clearSelectedGlobalPermissions() {
            this.selectedGlobalPermissions = [];
        },
        selectAllGlobalPermissions() {
            this.selectedGlobalPermissions = [...this.allGlobalPermissions];
        },
        updateHasChanged() {
            let original = JSON.stringify(this.originalConfig);
            let current = JSON.stringify(this.getCurrentUiRoles());
            // console.log("hasChanged original ", original);
            // console.log("hasChanged current ", current);
            // console.log("updateHasChanged", (original != current));
            return this.hasChanged = (original != current);
        },
        async save() {
            // reload the available-labels in case they have changed since we've loaded them and update the roles
            let fromApi = await api.getRolesConfig();
            let current = this.getCurrentUiRoles();
            current["available-labels"] = fromApi["available-labels"];
            await api.setRolesConfig(current);
            this.reset();
        },
        cancel() {
            this.reset();
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        labels() {
            return this.allLabels;
        },
        isLabelsListEmpty() {
            return this.allLabels.length == 0;
        },
        labelPermissions() {
            return this.allLabelPermissions;
        },
        globalPermissions() {
            return this.allGlobalPermissions;
        },
        allRoles() {
            if (this.currentConfig && "roles" in this.currentConfig) {
                return Object.keys(this.currentConfig["roles"]).sort();
            } else {
                return [];
            }
        },

    },
}
</script>
<template>
    <div class="settings">
        <div class="container">
            <h5>{{ $t('settings.permissions') }}</h5>
            <p class="instructions" v-html="$t('settings.permissions_global_instructions_html')"></p>
            <div class="row">
                <div class="col-3 border-end border-3">
                    <h6><i class="bi bi-people-fill"></i> {{ $t('settings.roles_title') }}
                        <button class="btn btn-sm bi bi-info-circle" data-bs-toggle="popover" data-bs-trigger="focus"
                            :data-bs-content="$t('settings.roles_description_html')"></button>
                    </h6>
                </div>
                <div class="col-9">
                    <h6>{{ $t('settings.labels_based_permissions_title') }}</h6>
                </div>
            </div>
            <div class="row">
                <div class="col-3 border-end border-3">
                </div>
                <div class="col-3">
                    <h6><i class="bi bi-key-fill"></i>{{ $t('settings.permissions') }}
                        <button class="btn btn-sm bi bi-info-circle" data-bs-toggle="popover" data-bs-trigger="focus"
                            :data-bs-content="$t('settings.permissions_instructions')"></button>
                    </h6>
                </div>
                <div class="col-6">
                    <h6><i class="bi bi-tags-fill"></i> {{ $t('settings.labels_title') }}
                        <button class="btn btn-sm bi bi-info-circle" data-bs-toggle="popover" data-bs-trigger="focus"
                            :data-bs-content="$t('settings.labels_description_html')"></button>
                    </h6>
                </div>
            </div>
            <div class="row">
                <div class="col-3 border-end border-3">
                </div>
                <div class="col-3 ">
                    <p class="">
                        <button type="button" class="btn btn-select btn-secondary" @click="selectAllLabelPermissions()"
                            data-bs-toggle="tooltip" :title="$t('settings.select_all_label_permissions')"><i
                                class="bi bi-list-check"></i></button>
                        <button type="button" class="btn btn-select btn-secondary"
                            @click="clearSelectedLabelPermissions()" data-bs-toggle="tooltip"
                            :title="$t('settings.permissions_clear_selection')"><i class="bi bi-x-circle"></i></button>
                    </p>
                </div>
                <div class="col-6">
                    <p class="">
                        <button type="button" class="btn btn-select btn-secondary" @click="selectAllLabels()"
                            :disabled="hasAllLabelsAllowed" data-bs-toggle="tooltip"
                            :title="$t('settings.labels_select_all_currents')"><i class="bi bi-list-check"></i></button>
                        <button type="button" class="btn btn-select btn-secondary" @click="clearSelectedLabels()"
                            :disabled="hasAllLabelsAllowed" data-bs-toggle="tooltip"
                            :title="$t('settings.labels_clear_selection')"><i class="bi bi-x-circle"></i></button>
                    </p>
                </div>
            </div>

            <div class="row">
                <div class="col-3 border-end border-3 permissions-body" style="height:auto">
                    <div class="list-group">
                        <button v-for="role in this.allRoles" :key="role" type="button"
                            class="list-group-item list-group-item-action" :class="{ 'active': isRoleSelected(role) }"
                            @click="selectRole(role)">{{ role }}</button>
                    </div>
                </div>
                <div class="col-3 permissions-body">
                    <div class="list-group">
                        <label v-for="perm in labelPermissions" :key="perm" class="list-group-item"
                            :class="{ 'active': isPermissionSelected(perm, 'label') }"
                            @click="toggleSelectedPermission($event, perm, 'label')">
                            {{ perm }}
                        </label>
                    </div>
                </div>
                <div class="col-6 mb-3">
                    <div class="form-check">
                        <input class="form-check-input" type="checkbox" value="" id="all_future_labels"
                            v-model="hasAllLabelsAllowed">
                        <label class="form-check-label" for="all_future_labels">
                            {{ $t('settings.allow_all_currents_and_futures_labels') }}
                        </label>
                    </div>
                    <div class="labels-list" :class="{ 'd-none': isLabelsListEmpty }">
                        <button v-for="label in labels" :key="label" @click="toggleSelectedLabel($event, label)"
                            class="labels-item border d-flex justify-content-between align-items-center"
                            :disabled="hasAllLabelsAllowed"
                            :class="{ 'is-label-selected': isLabelSelected(label), 'is-label-not-selected': !isLabelSelected(label) }">
                            <span class="list-group-item-with-button-content">{{ label }}</span>
                        </button>
                    </div>
                    <div class="labels-list mt-4" :class="{ 'd-none': !isLabelsListEmpty }">
                        <p class="w-100 text-center">{{ $t('settings.labels_list_empty') }}</p>
                    </div>
                </div>
            </div>
            <div class="row">
                <div class="col-3 pt-3 border-end border-3">
                </div>
                <div class="col-3 pt-3 permission-body-global border-top border-3">
                    <h6>{{ $t('settings.global_permissions_title') }}</h6>
                    <p class="">
                        <button type="button" class="btn btn-select btn-secondary" @click="selectAllGlobalPermissions()"
                            data-bs-toggle="tooltip" :title="$t('settings.select_all_global_permissions')"><i
                                class="bi bi-list-check"></i></button>
                        <button type="button" class="btn btn-select btn-secondary"
                            @click="clearSelectedGlobalPermissions()" data-bs-toggle="tooltip"
                            :title="$t('settings.permissions_clear_selection')"><i class="bi bi-x-circle"></i></button>
                    </p>
                    <div class="list-group">
                        <label v-for="perm in globalPermissions" :key="perm" class="list-group-item"
                            :class="{ 'active': isPermissionSelected(perm, 'global') }"
                            @click="toggleSelectedPermission($event, perm, 'global')">
                            {{ perm }}
                        </label>
                    </div>
                </div>
                <div class="col-6 pt-3 permission-body-global border-top border-3">
                </div>
            </div>



            <div class="d-flex justify-content-end mt-2 mb-4">
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

.w-15 {
    width: 15%;
}

.instructions {
    margin-left: 1rem;
}

.instructions-text {
    font-size: smaller;
}

.permissions-body {
    height: 30vh;
    overflow: overlay;
}

.labels-list {
    display: flex;
    flex-flow: row wrap;
    max-height: 35vh;
    margin: 2px;
    overflow: overlay;
}

.labels-item {
    flex: 0 1 auto;
    min-width: 10vw;
    height: 3rem;
    padding-left: 0.5rem;
    padding-bottom: 0;
    padding-right: 0.2rem;
    padding-top: 0;
    margin: 3px;
}

.w-15 {
    width: 15%;
}

.btn-select {
    margin: 0.2rem;
    padding-left: 0.40rem;
    padding-right: 0.40rem;
    padding-top: 0.15rem;
    padding-bottom: 0.15rem;
    font-size: larger;
}

.is-label-selected {
    background-color: var(--bs-list-group-active-bg);

}

.is-label-not-selected {
    background-color: var(--bs-body-bg);
}
</style>