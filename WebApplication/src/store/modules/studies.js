import api from "../../orthancApi"
import SourceType from "../../helpers/source-type";
import store from "../../store"

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
    labelFilters: [],
    orderByFilters: [],
    statistics: {},
    isSearching: false,
    selectedStudiesIds: [],
    selectedStudies: [],
    sourceType: SourceType.LOCAL_ORTHANC,
    remoteSource: null,
})

function insert_wildcards(initialValue) {
    // 'filter'   -> *filter* (by default, adds the wildcard before and after)
    // '"filter'  -> filter*  (a double quote means "no wildcard")
    // 'filter"'  -> *filter  (a double quote means "no wildcard")
    // '"filter"' -> filter  (= exact match)
    let finalValue = '*' + initialValue.replaceAll('"', '*') + '*';
    return finalValue.replaceAll('**', '');
}

async function get_studies_shared(context, append) {
    const commit = context.commit;
    const state = context.state;
    const getters = context.getters;

    if (!append) {
        commit('setStudiesIds', { studiesIds: [] });
        commit('setStudies', { studies: [] });
    }

    try {
        commit('setIsSearching', { isSearching: true});
        let studies = [];

        if (state.sourceType == SourceType.LOCAL_ORTHANC) {
            let orderBy = [...state.orderByFilters];
            if (state.orderByFilters.length == 0) {
                orderBy.push({'Type': 'Metadata', 'Key': 'LastUpdate', 'Direction': 'DESC'})
            }
            let since = (append ? state.studiesIds.length : null);
            studies = (await api.findStudies(getters.filterQuery, state.labelFilters, "All", orderBy, since));
        } else if (state.sourceType == SourceType.REMOTE_DICOM || state.sourceType == SourceType.REMOTE_DICOM_WEB) {
            // make sure to fill all columns of the StudyList
            let filters = {
                "PatientBirthDate": "",
                "PatientID": "",
                "AccessionNumber": "",
                "PatientBirthDate": "",
                "StudyDescription": "",
                "StudyDate": ""
            };

            // request values for e.g ModalitiesInStudy, NumberOfStudyRelatedSeries
            for (let t of store.state.configuration.requestedTagsForStudyList) {
                filters[t] = "";
            }

            // overwrite with the filtered values
            for (const [k, v] of Object.entries(getters.filterQuery)) {
                filters[k] = v;
            }

            let remoteStudies;
            if (state.sourceType == SourceType.REMOTE_DICOM) {
                remoteStudies = (await api.remoteDicomFind("Study", state.remoteSource, filters, true /* isUnique */));
            } else if (state.sourceType == SourceType.REMOTE_DICOM_WEB) {
                remoteStudies = (await api.qidoRs("Study", state.remoteSource, filters, true /* isUnique */));
            }
            
            // copy the tags in MainDicomTags, ... to have a common study structure between local and remote studies
            studies = remoteStudies.map(s => { return {"MainDicomTags": s, "PatientMainDicomTags": s, "RequestedTags": s, "ID": s["StudyInstanceUID"]} });
        }

        studies = studies.map(s => {return {...s, "sourceType": state.sourceType} });
        let studiesIds = studies.map(s => s['ID']);

        if (!append) {
            commit('setStudiesIds', { studiesIds: studiesIds });
            commit('setStudies', { studies: studies });
        } else {
            commit('extendStudiesIds', { studiesIds: studiesIds });
            commit('extendStudies', { studies: studies });
        }
    } catch (err) {
        console.log("Find studies cancelled", err);
    } finally {
        commit('setIsSearching', { isSearching: false});
    }
}

///////////////////////////// GETTERS
const getters = {
    filterQuery: (state) => {
        let query = {};
        for (const [k, v] of Object.entries(state.dicomTagsFilters)) {
            if (v && v.length >= 1 && k != 'labels') {
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
    isFilterEmpty: (state) => {
        for (const [k, v] of Object.entries(state.dicomTagsFilters)) {
            if (k == 'ModalitiesInStudy') {
                if (v && v != 'NONE') {
                    return false;
                }
            } else {
                if (v && v.length >= 1) {
                    return false;
                }
            }
        }
        // dicomTags filter is empty, check the labels
        return (!state.labelsFilter || state.labelsFilter.length == 0);
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
    extendStudiesIds(state, { studiesIds }) {
        state.studiesIds.push(...studiesIds);
    },
    extendStudies(state, { studies }) {
        state.studies.push(...studies);
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
        state.labelFilters = [];
    },
    setLabelFilters(state, { labels }) {
        state.labelFilters = [];
        for (let f of labels) {
            state.labelFilters.push(f);
        }
    },
    setOrderByFilters(state, { orderBy }) {
        console.log("setOrderByFilters");
        state.orderByFilters = [];
        for (let f of orderBy) {
            state.orderByFilters.push(f);
        }
    },
    setSource(state, { sourceType, remoteSource }) {
        state.sourceType = sourceType;
        state.remoteSource = remoteSource;
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
            state.selectedStudiesIds = [...state.studiesIds];
            state.selectedStudies = [...state.studies];
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
    async updateLabelFilterNoReload({ commit }, payload) {
        const labels = payload['labels'];
        commit('setLabelFilters', { labels })
    },
    async updateOrderByNoReload({ commit }, payload) {
        const orderBy = payload['orderBy'];
        commit('setOrderByFilters', { orderBy })
    },
    async updateOrderBy({ commit }, payload) {
        const orderBy = payload['orderBy'];
        commit('setOrderByFilters', { orderBy })

        this.dispatch('studies/reloadFilteredStudies');
    },

    async updateSource({ commit }, payload) {
        const sourceType = payload['source-type'];
        const remoteSource = payload['remote-source'];
        commit('setSource', { sourceType, remoteSource });
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
    async extendFilteredStudies({ commit, getters, state }) {
        get_studies_shared({ commit, getters, state }, true);
    },
    async reloadFilteredStudies({ commit, getters, state }) {
        get_studies_shared({ commit, getters, state }, false);
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
        const reloadStats = payload['reloadStats'];
        commit('addStudy', { studyId: studyId, study: study });
        if (reloadStats) {
            this.dispatch('studies/loadStatistics');
        }
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