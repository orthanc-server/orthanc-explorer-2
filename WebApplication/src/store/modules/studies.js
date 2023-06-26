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
    dicomTagsFilters: {..._clearedFilter},
    labelsFilter: [],
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
        for (const [k, v] of Object.entries(state.dicomTagsFilters)) {
            if (v && v.length >= 1) {
                if (['StudyDate', 'PatientBirthDate'].indexOf(k) != -1) {
                    // for dates, accept only exactly 8 chars
                    if (v.length >= 8) {
                        query[k] = v;
                    }
                } else if (['StudyInstanceUID', 'ModalitiesInStudy'].indexOf(k) != -1) {
                    // exact match
                    query[k] = v;
                } else {
                    // wildcard match for all other fields
                    query[k] = insert_wildcards(v);
                }
            }
        }
        return query;
    },
    isFilterEmpty: (state, getters) => {
        return Object.keys(getters.filterQuery).length == 0 && (!state.labelsFilter || state.labelsFilter.length == 0);
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
        } else {
            for (let s in state.studies) {
                if (state.studies[s].ID == studyId) {
                    state.studies[s] = study;
                }
            }
        }
    },
    setFilter(state, { dicomTagName, value }) {
        state.dicomTagsFilters[dicomTagName] = value;
    },
    clearFilter(state) {
        state.dicomTagsFilters = {..._clearedFilter};
        state.labelsFilter = [];
    },
    setLabelsFilter(state, { labels }) {
        state.labelsFilter = [];
        for (let f of labels) {
            state.labelsFilter.push(f);
        }
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
    refreshStudyLabels(state, {studyId, labels}) {
        for (const i in state.studies) {
            if (state.studies[i].ID == studyId) {
                state.studies[i].Labels = labels
            }
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
    async updateLabelsFilterNoReload({ commit }, payload) {
        const labels = payload['labels'];
        commit('setLabelsFilter', { labels })
    },
    async clearFilter({ commit, state }) {
        commit('clearFilter');

        this.dispatch('studies/reloadFilteredStudies');
    },
    async clearFilterNoReload({ commit }) {
        commit('clearFilter');
    },
    async clearStudies({ commit }) {
        commit('setStudiesIds', { studiesIds: [] });
        commit('setStudies', { studies: [] });
    },
    async reloadFilteredStudies({ commit, getters, state }) {
        commit('setStudiesIds', { studiesIds: [] });
        commit('setStudies', { studies: [] });

        if (!getters.isFilterEmpty) {
            try {
                commit('setIsSearching', { isSearching: true});
                const studies = (await api.findStudies(getters.filterQuery, state.labelsFilter, "All"));
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
    async reloadStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        const study = payload['study'];
        commit('addStudy', { studyId: studyId, study: study });
        this.dispatch('studies/loadStatistics');
    },
    async refreshStudiesLabels({ commit }, payload) {
        const studiesIds = payload['studiesIds'];

        for (const studyId of studiesIds) {
            const labels = await api.getLabels(studyId);

            commit('refreshStudyLabels', { studyId: studyId, labels: labels});
        }
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