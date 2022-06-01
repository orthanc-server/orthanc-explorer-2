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
    isSearching: false
})

///////////////////////////// GETTERS
const getters = {
    filterQuery: (state, getters) => {
        let query = {};
        if (state.filters.StudyDate.length >= 8) {
            query["StudyDate"] = state.filters.StudyDate;
        }
        if (state.filters.AccessionNumber.length >= 1) {
            query["AccessionNumber"] = "*" + state.filters.AccessionNumber + "*";
        }
        if (state.filters.PatientID.length >= 1) {
            query["PatientID"] = "*" + state.filters.PatientID + "*";
        }
        if (state.filters.PatientName.length >= 1) {
            query["PatientName"] = "*" + state.filters.PatientName + "*";
        }
        if (state.filters.PatientBirthDate.length >= 8) {
            query["PatientBirthDate"] = state.filters.PatientBirthDate;
        }
        if (state.filters.StudyDescription.length >= 1) {
            query["StudyDescription"] = "*" + state.filters.StudyDescription + "*";
        }
        if (state.filters.StudyInstanceUID.length >= 1) {
            query["StudyInstanceUID"] = state.filters.StudyInstanceUID;
        }
        if (state.filters.ModalitiesInStudy.length >= 1) {
            query["ModalitiesInStudy"] = state.filters.ModalitiesInStudy;
        }
        return query;
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
        state.studies = state.studies.map(s => s["ID"] != studyId);
    },
    setStatistics(state, {statistics}) {
        state.statistics = statistics;
    },
    setIsSearching(state, {isSearching}) {
        state.isSearching = isSearching;
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
        try {
            commit('setIsSearching', { isSearching: true});
            const studies = (await api.findStudies(getters.filterQuery)).data;
            let studiesIds = studies.map(s => s['ID']);
            commit('setStudiesIds', { studiesIds: studiesIds });
            commit('setStudies', { studies: studies });
        } catch (err) {
            console.log("Find studies cancelled");
        } finally {
            commit('setIsSearching', { isSearching: false});
        }
    },
    async cancelSearch() {
        await api.cancelFindStudies();
    },
    async loadStatistics({ commit }) {
        const statistics = (await api.getStatistics()).data;
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
    }

}



///////////////////////////// EXPORT

export default {
    namespaced: true,
    state,
    getters,
    mutations,
    actions,
}