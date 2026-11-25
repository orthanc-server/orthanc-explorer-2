<script>
import { mapState, mapGetters } from "vuex"

export default {
    props: {
        buttonId: String,
        buttonClass: { default: "btn btn-default btn-sm filter-button dropdown-toggle" },
        dropdownStyle: Object,
    },
    data() {
        return {
            showModalityDropdown: false,
            modalityDropdownStyle: {},
            filterModalities: {},
            allModalitiesSelected: true,
            noneModalitiesSelected: false,
        }
    },
    created() {
        this.messageBus.on('update-modalities-filter', this.updateModalitiesFilter);
        this.messageBus.on('clear-modalities-filter', this.clearModalityFilter);
        this._onDocClick = (ev) => this.onDocumentClick(ev);
        this._onDocTouch = (ev) => this.onDocumentClick(ev);
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
        })
    },
    beforeUnmount() {
        this.removeOutsideClickListener();
    },
    watch: {
        isConfigurationLoaded(newValue, oldValue) {
            this.init();
        },
        filterModalities: {
            handler(newValue, oldValue) {
                let modalitiesFilter = this.getModalityFilter(); // to update all/none status
                this.messageBus.emit('modalities-filter-updated', modalitiesFilter);
            },
            deep: true
        },
    },
    methods: {
        init() {
            this.initModalityFilter();
        },
        addOutsideClickListener() {
            document.addEventListener('click', this._onDocClick);
            // click events are unreliable on touch devices
            document.addEventListener('touchstart', this._onDocTouch);
        },
        removeOutsideClickListener() {
            document.removeEventListener('click', this._onDocClick);
            document.removeEventListener('touchstart', this._onDocTouch);
        },        
        clearModalityFilter() {
            // console.log("StudyList: clearModalityFilter", this.updatingFilterUi);
            if (this.uiOptions.StudyListColumns) {
                for (const modality of this.uiOptions.ModalitiesFilter) {
                    this.filterModalities[modality] = true;
                }
            }
        },
        updateModalitiesFilter(value) {
            const modalities = value.split('\\');
            if (modalities.length > 0) {
                let allModalitiesInFilter = true;
                let noneModalitiesInFilter = true;
                for (const modality of this.uiOptions.ModalitiesFilter) {
                    const isInFilter = modalities.indexOf(modality) != -1
                    this.filterModalities[modality] = isInFilter;
                    allModalitiesInFilter &&= isInFilter;
                    noneModalitiesInFilter &&= !!isInFilter;
                }
                this.allModalitiesSelected = allModalitiesInFilter;
                this.noneModalitiesSelected = noneModalitiesInFilter;
            }
        },
        initModalityFilter() {
            // console.log("StudyList: initModalityFilter", this.updatingFilterUi);
            this.initializingModalityFilter = true;
            this.filterModalities = {};
            for (const modality of this.uiOptions.ModalitiesFilter) {
                this.filterModalities[modality] = true;
            }
            this.initializingModalityFilter = false;
        },
        getModalityFilter() {
            if (this.filterModalities === undefined) {
                return "";
            }

            let modalityFilter = "";
            let allSelected = true;
            let selected = [];

            // console.log("getModalityFilter", this.filterModalities);
            for (const [key, value] of Object.entries(this.filterModalities)) {
                allSelected &= value;
                if (value) {
                    selected.push(key);
                }
            }
            if (allSelected) {
                this.allModalitiesSelected = true;
                this.noneModalitiesSelected = false;
                return "";
            } else if (selected.length == 0) {
                this.allModalitiesSelected = false;
                this.noneModalitiesSelected = true;
                return "NONE"; // something that will not match !
            } else {
                this.allModalitiesSelected = false;
                this.noneModalitiesSelected = false;
                return selected.join('\\');
            }
        },
        stop(ev) { 
            ev.stopPropagation(); 
        },
        async toggleAllNone(ev) {
            // only for all/none, other values are binded with v-model !
            const modality = ev.srcElement.getAttribute("data-value");
            let newValue = true;
            if (modality == "all") {
                newValue = true;
            } else if (modality == "none") {
                newValue = false;
            }

            for (const [key, value] of Object.entries(this.filterModalities)) {
                this.filterModalities[key] = newValue;
            }

            this.getModalityFilter(); // to update the state of "all"/"none"
        },
        toggleModalityDropdown(ev) {
            ev.stopPropagation();
            this.showModalityDropdown = !this.showModalityDropdown;
            if (this.showModalityDropdown) {
                this.addOutsideClickListener();
                this.$nextTick(() => {
                    this.positionModalityDropdown();
                });
            } else {
                this.removeOutsideClickListener();
            }
        },
        positionModalityDropdown() {
            let btn = document.getElementById(this.buttonId);
            if (!btn) {
                return;
            }
            const rect = btn.getBoundingClientRect();
            const maxHeight = Math.max(window.innerHeight - rect.bottom - 10, 150);
            const margin = 8;
            const menu = document.getElementById('modality-filter-dropdown');
            const menuWidth = menu ? menu.offsetWidth : 0;
            const style = {
                position: 'fixed',
                top: (rect.bottom + 2) + 'px',
                maxHeight: maxHeight + 'px',
                overflowY: 'auto',
                overscrollBehavior: 'contain',
                WebkitOverflowScrolling: 'touch',
                zIndex: 1050
            };
            // The dropdown extends leftward from the button by default; when the
            // button sits near the left edge, anchor and clamp it from the left instead.
            if (menuWidth && rect.right < menuWidth + margin) {
                const left = Math.min(
                    Math.max(rect.left, margin),
                    Math.max(margin, window.innerWidth - margin - Math.min(menuWidth, window.innerWidth - 2 * margin))
                );
                style.left = left + 'px';
                style.maxWidth = Math.min(menuWidth, window.innerWidth - 2 * margin) + 'px';
                style.width = 'max-content';
            } else {
                style.right = (window.innerWidth - rect.right) + 'px';
            }
            this.modalityDropdownStyle = style;
        },
        onDocumentClick(ev) {
            if (!this.showModalityDropdown) return;
            const dropdown = document.getElementById('modality-filter-dropdown');
            let btn = document.getElementById(this.buttonId);

            if (dropdown && dropdown.contains(ev.target)) {
                return;
            }
            if (btn && btn.contains(ev.target)) {
                return;
            }
            this.showModalityDropdown = false;
            this.removeOutsideClickListener();
        },
        closeModalityFilter(ev) {
            this.showModalityDropdown = false;
            this.removeOutsideClickListener();
            ev.preventDefault();
            ev.stopPropagation();
        },
    }
}
</script>

<template>
    <div class="dropdown">
        <button type="button" :class="buttonClass" @click="toggleModalityDropdown($event)"
            :id="buttonId" :aria-expanded="showModalityDropdown">
            <span class="fa fa-list"></span>&nbsp;<span class="caret"></span>
        </button>
        <Teleport to="body">
            <ul v-if="showModalityDropdown" class="dropdown-menu show" :style="modalityDropdownStyle"
                @click="stop" id="modality-filter-dropdown">
                <li><label class="dropdown-item"><input type="checkbox" data-value="all"
                        @click="toggleAllNone" :checked="allModalitiesSelected" />&nbsp;{{ $t('all_modalities') }}</label></li>
                <li><label class="dropdown-item"><input type="checkbox" data-value="none"
                        @click="toggleAllNone" :checked="noneModalitiesSelected" />&nbsp;{{ $t('no_modalities') }}</label></li>
                <li><hr class="dropdown-divider"></li>
                <li v-for="(isSelected, modality) of filterModalities" :key="modality">
                    <label class="dropdown-item"><input type="checkbox" :data-value="modality"
                        v-model="filterModalities[modality]" />&nbsp;{{ modality }}</label>
                </li>
                <li><button class="btn btn-primary mx-5" @click="closeModalityFilter($event)">{{ $t('close') }}</button></li>
            </ul>
        </Teleport>
    </div>
</template>