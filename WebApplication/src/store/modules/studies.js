import api from "../../orthancApi"

const _clearedFilter = {
    StudyDate : "",
    AccessionNumber: "",
    PatientID: "",
    PatientName: "",
    PatientBirthDate: "",
    StudyDescription: "",
    StudyInstanceUID: "",
    ModalitiesInStudy: "",
}

///////////////////////////// STATE
const state = () => ({
    studies: [],  // studies as returned by tools/find
    studiesIds: [],
    filters: {..._clearedFilter},
    statistics: {},
    isSearching: false,
    selectedStudiesIds: [],
    selectedStudies: []
})

function insert_wildcards(initialValue) {
    // 'filter'   -> *filter* (by default, adds the wildcard before and after)
    // '"filter'  -> filter*  (a double quote means "no wildcard")
    // 'filter"'  -> *filter  (a double quote means "no wildcard")
    // '"filter"' -> filter  (= exact match)
    let finalValue = '*' + initialValue.replaceAll('"', '*') + '*';
    return finalValue.replaceAll('**', '');
}

///////////////////////////// GETTERS
const getters = {
    filterQuery: (state) => {
        let query = {};
        if (state.filters.StudyDate.length >= 8) {
            query["StudyDate"] = state.filters.StudyDate;
        }
        if (state.filters.AccessionNumber.length >= 1) {
            query["AccessionNumber"] = insert_wildcards(state.filters.AccessionNumber);
        }
        if (state.filters.PatientID.length >= 1) {
            query["PatientID"] = insert_wildcards(state.filters.PatientID);
        }
        if (state.filters.PatientName.length >= 1) {
            query["PatientName"] = insert_wildcards(state.filters.PatientName);
        }
        if (state.filters.PatientBirthDate.length >= 8) {
            query["PatientBirthDate"] = state.filters.PatientBirthDate;
        }
        if (state.filters.StudyDescription.length >= 1) {
            query["StudyDescription"] = insert_wildcards(state.filters.StudyDescription);
        }
        if (state.filters.StudyInstanceUID.length >= 1) {
            query["StudyInstanceUID"] = state.filters.StudyInstanceUID;
        }
        if (state.filters.ModalitiesInStudy.length >= 1) {
            query["ModalitiesInStudy"] = state.filters.ModalitiesInStudy;
        }
        return query;
    },
    isFilterEmpty: (state, getters) => {
        return Object.keys(getters.filterQuery).length == 0;
    }
}

///////////////////////////// MUTATIONS

const mutations = {
    setStudiesIds(state, { studiesIds }) {
        state.studiesIds = studiesIds;
    },
    setStudies(state, { studies }) {
        state.studies = studies;
    },
    addStudy(state, { studyId, study }) {
        if (!state.studiesIds.includes(studyId)) {
            state.studiesIds.push(studyId);
            state.studies.push(study);
        }

    },
    setFilter(state, { dicomTagName, value }) {
        state.filters[dicomTagName] = value;
    },
    clearFilter(state) {
        state.filters = {..._clearedFilter};
    },
    deleteStudy(state, {studyId}) {
        const pos = state.studiesIds.indexOf(studyId);
        if (pos >= 0) {
            state.studiesIds.splice(pos, 1);
        }
        state.studies = state.studies.filter(s => s["ID"] != studyId);

        // also delete from selection
        const pos2 = state.selectedStudiesIds.indexOf(studyId);
        if (pos2 >= 0) {
            state.selectedStudiesIds.splice(pos, 1);
        }
    },
    setStatistics(state, {statistics}) {
        state.statistics = statistics;
    },
    setIsSearching(state, {isSearching}) {
        state.isSearching = isSearching;
    },
    selectStudy(state, {studyId, isSelected}) {
        if (isSelected && !state.selectedStudiesIds.includes(studyId)) {
            state.selectedStudiesIds.push(studyId);
            state.selectedStudies = state.selectedStudies.concat(state.studies.filter(s => state.selectedStudiesIds.includes(s["ID"])))
        } else if (!isSelected) {
            const pos = state.selectedStudiesIds.indexOf(studyId);
            if (pos >= 0) {
                state.selectedStudiesIds.splice(pos, 1);
                state.selectedStudies = state.selectedStudies.filter(s => s["ID"] != studyId);
            }
        }
    },
    selectAllStudies(state, {isSelected}) {
        if (isSelected) {
            state.selectedStudiesIds = state.studiesIds;
            state.selectedStudies = state.studies;
        } else {
            state.selectedStudiesIds = [];
            state.selectedStudies = [];
        }
       
    }
}

///////////////////////////// ACTIONS

const actions = {
    async initialLoad({ commit, state}) {
        this.dispatch('studies/loadStatistics');
    },
    async updateFilter({ commit }, payload) {
        const dicomTagName = payload['dicomTagName'];
        const value = payload['value'];
        commit('setFilter', { dicomTagName, value })

        this.dispatch('studies/reloadFilteredStudies');
    },
    async updateFilterNoReload({ commit }, payload) {
        const dicomTagName = payload['dicomTagName'];
        const value = payload['value'];
        commit('setFilter', { dicomTagName, value })
    },
    async clearFilter({ commit, state }) {
        commit('clearFilter');

        this.dispatch('studies/reloadFilteredStudies');
    },
    async clearFilterNoReload({ commit }) {
        commit('clearFilter');
    },
    async reloadFilteredStudies({ commit, getters }) {
        commit('setStudiesIds', { studiesIds: [] });
        commit('setStudies', { studies: [] });

        if (getters.isFilterEmpty && this.state.configuration.uiOptions.StudyListEmptyIfNoSearch) {
            return;
        } else {
            try {
                commit('setIsSearching', { isSearching: true});
                const studies = (await api.findStudies(getters.filterQuery));
                let studiesIds = studies.map(s => s['ID']);
                commit('setStudiesIds', { studiesIds: studiesIds });
                commit('setStudies', { studies: studies });
            } catch (err) {
                console.log("Find studies cancelled");
            } finally {
                commit('setIsSearching', { isSearching: false});
            }
        }
    },
    async cancelSearch() {
        await api.cancelFindStudies();
    },
    async loadStatistics({ commit }) {
        const statistics = (await api.getStatistics());
        commit('setStatistics', { statistics: statistics });
    },
    async deleteStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        commit('deleteStudy', { studyId });
        this.dispatch('studies/loadStatistics');
    },
    async addStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        const study = payload['study'];
        commit('addStudy', { studyId: studyId, study: study });
        this.dispatch('studies/loadStatistics');
    },
    async selectStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        const isSelected = payload['isSelected'];
        commit('selectStudy', { studyId: studyId, isSelected: isSelected});
    },
    async selectAllStudies({ commit }, payload) {
        const isSelected = payload['isSelected'];
        commit('selectAllStudies', { isSelected: isSelected});
    },
}



///////////////////////////// EXPORT

export default {
    namespaced: true,
    state,
    getters,
    mutations,
    actions,
}