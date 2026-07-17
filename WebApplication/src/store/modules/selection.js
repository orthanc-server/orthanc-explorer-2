import api from "../../orthancApi"
import SourceType from "../../helpers/source-type";
import store from "../../store"
import SelectionStatus from "../../helpers/selection-status";
import { create } from "axios";

///////////////////////////// STATE
const state = () => ({
    hasBulkSelection: false,
    hasStudyOnlyBulkSelection: false,
    selectedResources:  {} // a dictionary 
    //  'studyId1': {
    //     'allSeriesSelected': true/false, 
    //     'selectedSeries' : {
    //       'seriesId1' : {
    //         'allInstancesSelected: true/false,
    //         'selectedInstances': [inst1Id, inst2Id]
    //       }, ...
    //     }, ...
    //  }, ...
    //}}
})



///////////////////////////// GETTERS
const getters = {
    getStudySelectionStatus: (state) => (studyId) => {
        if (studyId in state.selectedResources) {
            if (state.selectedResources[studyId]['allSeriesSelected']) {
                return SelectionStatus.FULL;
            } else {
                return SelectionStatus.PARTIAL;
            }
        }
        return SelectionStatus.NOT_SELECTED;
    },
    selectedResourcesIdsWithLevel: (state) => {
        let toReturn = [];
        for (const [studyId, study] of Object.entries(state.selectedResources)) {
            if (study['allSeriesSelected']) {
                toReturn.push({"Level": "Study", "ID" : studyId});
            } else {
                for (const [seriesId, series] of Object.entries(study['selectedSeries'])) {
                    if (series['allInstancesSelected']) {
                        toReturn.push({"Level": "Series", "ID" : seriesId});
                    } else {
                        for (const instanceId of series['selectedInstances']) {
                            toReturn.push({"Level": "Instance", "ID" : instanceId});
                        }
                    }
                }
            }
        }
        return toReturn;
    },
    selectedResourcesIds: (state) => (studiesOnly) => {
        if (studiesOnly) {
            return Object.keys(state.selectedResources);
        }

        let toReturn = [];
        for (const [studyId, study] of Object.entries(state.selectedResources)) {
            if (study['allSeriesSelected']) {
                toReturn.push(studyId);
            } else {
                for (const [seriesId, series] of Object.entries(study['selectedSeries'])) {
                    if (series['allInstancesSelected']) {
                        toReturn.push(seriesId);
                    } else {
                        toReturn = toReturn.concat(series['selectedInstances']);
                    }
                }
            }
        }
        return toReturn;
    },
    selectedResourcesDicomIds: (state) => (studiesOnly) => {
        if (!studiesOnly) {
            console.warn("TODO selectedResourcesDicomIds/studiesOnly");
            return [];
        }

        let toReturn = [];
        for (const [studyId, study] of Object.entries(state.selectedResources)) {
            const studiesInfo = store.state.studies.studies.filter((s) => s['ID'] == studyId);
            if (studiesInfo.length == 1) {
                toReturn.push(studiesInfo[0]['MainDicomTags']['StudyInstanceUID']);
            }
        }
        return toReturn;
    },
    selectedStudiesCount: (state) => { // studies that are at least partialy selected
        return Object.keys(state.selectedResources).length;
    },
    isStudiesFullSelection: (state) => {
        if (Object.keys(state.selectedResources).length == 0) {
            return false;
        }
        const allStudiesCount = store.state.studies.studies.length;
        let fullySelectedStudiesCount = 0;
        for (const [k, v] of Object.entries(state.selectedResources)) {
            if (v['allSeriesSelected']) {
                fullySelectedStudiesCount++;
            }
        }
        return fullySelectedStudiesCount == allStudiesCount;
    },
    isStudiesPartialSelection: (state, getters) => {
        if (Object.keys(state.selectedResources).length == 0) {
            return false;
        }
        return !getters.isStudiesFullSelection;
    },
    getSeriesSelectionStatus: (state, getters) => (studyId, seriesId) => {
        const parentStudySelectionStatus = getters.getStudySelectionStatus(studyId);
        if (parentStudySelectionStatus == SelectionStatus.FULL) {
            return SelectionStatus.FULL;
        } else if (parentStudySelectionStatus == SelectionStatus.NOT_SELECTED) {
            return SelectionStatus.NOT_SELECTED
        } else {
            const series = state.selectedResources[studyId]['selectedSeries'][seriesId];
            if (series === undefined) {
                return SelectionStatus.NOT_SELECTED;
            } else {
                if (series['allInstancesSelected']) {
                    return SelectionStatus.FULL
                } else {
                    return SelectionStatus.PARTIAL;    
                }
            }
        }
    },
    getSelectedSeriesCount: (state, getters) => (studyId, studySeries) => {
        const selectedStudy = state.selectedResources[studyId];
        if (selectedStudy === undefined) {
            return 0;
        } else {
            if (selectedStudy['allSeriesSelected']) {
                return studySeries.length;
            } else {
                return Object.keys(selectedStudy['selectedSeries']).length;
            }
        }
    },
    getSelectedInstancesCount: (state, getters) => (studyId, seriesId, seriesInstances) => {
        const selectedStudy = state.selectedResources[studyId];
        if (selectedStudy === undefined) {
            return 0;
        } else {
            if (selectedStudy['allSeriesSelected']) {
                return seriesInstances.length;
            } else if (seriesId in selectedStudy['selectedSeries']) {
                if (selectedStudy['selectedSeries'][seriesId]['allInstancesSelected']) {
                    return seriesInstances.length;
                } else {
                    return Object.keys(selectedStudy['selectedSeries'][seriesId]['selectedInstances']).length;
                }
            }
        }
    },
    getInstanceSelectionStatus: (state, getters) => (studyId, seriesId, instanceId) => {
        const parentSeriesSelectionStatus = getters.getSeriesSelectionStatus(studyId, seriesId);
        if (parentSeriesSelectionStatus == SelectionStatus.FULL) {
            return SelectionStatus.FULL;
        } else if (parentSeriesSelectionStatus == SelectionStatus.NOT_SELECTED) {
            return SelectionStatus.NOT_SELECTED;
        } else {
            if (state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].indexOf(instanceId) == -1) {
                return SelectionStatus.NOT_SELECTED;
            } else {
                SelectionStatus.FULL;
            }
        }
    },


}

///////////////////////////// MUTATIONS

/// helpers

function _setFullSelectedStudy(context, studyId) {
    let state = context.state;
    if (!(studyId in state.selectedResources)) {
        state.selectedResources[studyId] = {
            'allSeriesSelected': true
        }
    }
}

function _setPartialySelectedStudy(context, studyId) {
    let state = context.state;
    if (!(studyId in state.selectedResources)) {
        state.selectedResources[studyId] = {
            'allSeriesSelected': false,
            'selectedSeries': {}
        }
    }
    if (!('selectedSeries' in state.selectedResources[studyId])) {
        state.selectedResources[studyId]['allSeriesSelected'] = false;
        state.selectedResources[studyId]['selectedSeries'] = {}
    }
}

function _removeStudy(context, studyId) {
    let state = context.state;
    delete state.selectedResources[studyId];
}

function _setPartialySelectedSeries(context, studyId, seriesId) {
    let state = context.state;
    _setPartialySelectedStudy(context, studyId);
    state.selectedResources[studyId]['selectedSeries'][seriesId] = {
        'allInstancesSelected': false,
        'selectedInstances': []
    }
}

function _setFullSelectedSeries(context, studyId, seriesId) {
    let state = context.state;
    _setPartialySelectedStudy(context, studyId);
    state.selectedResources[studyId]['selectedSeries'][seriesId] = {
        'allInstancesSelected': true
    }
}

function _simplifyAtSeriesLevel(context, studyId, seriesId, instancesCount) {
    let state = context.state;

    // simplification at this series level
    if (studyId in state.selectedResources && 'selectedSeries' in state.selectedResources[studyId] 
        && seriesId in state.selectedResources[studyId]['selectedSeries'] && 'selectedInstances' in state.selectedResources[studyId]['selectedSeries'][seriesId]) {
        if (state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].length == 0) {
            delete state.selectedResources[studyId]['selectedSeries'][seriesId];
        } else if (state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].length == instancesCount) {
            state.selectedResources[studyId]['selectedSeries'][seriesId] = {
                'allInstancesSelected': true
            }
        }
    }
}

function _simplifyAtStudyLevel(context, studyId, seriesCount) {
    let state = context.state;

    // simplification at study level
    if (studyId in state.selectedResources && 'selectedSeries' in state.selectedResources[studyId]) {
        let fullySelectedSeriesCount = 0;
        for (const [k, v] of Object.entries(state.selectedResources[studyId]['selectedSeries'])) {
            if (v['allInstancesSelected']) {
                fullySelectedSeriesCount++;
            }
        }
        if (fullySelectedSeriesCount == seriesCount) {
            state.selectedResources[studyId] = {
                'allSeriesSelected': true
            }
        } else if (Object.keys(state.selectedResources[studyId]['selectedSeries']).length == 0) { // we have removed the last series from the study
            delete state.selectedResources[studyId];
        }
    }
}

function _updateBulkSelection(context) {
    let state = context.state;
    state.hasBulkSelection = Object.keys(state.selectedResources).length > 0;

    state.hasStudyOnlyBulkSelection = state.hasBulkSelection;
    for (const [k, v] of Object.entries(state.selectedResources)) {
        if (!v['allSeriesSelected']) {
            state.hasStudyOnlyBulkSelection = false;
        }
    }
}

function _clearSelection(context) {
    let state = context.state;
    state.selectedResources = {};
}

const mutations = {
    selectStudy(state, { studyId, isSelected }) { // completely selects/unselected a study
        if (isSelected) {
            _setFullSelectedStudy({state}, studyId);
        } else if (!isSelected) {
            _removeStudy({state}, studyId);
        }
        _updateBulkSelection({state});
    },
    removeStudy(state, { studyId }) { // when a study has been deleted, we need to remove it from the selection.
        _removeStudy({state}, studyId);
        _updateBulkSelection({state});
    },
    selectAllStudies(state, { isSelected }) {
        if (isSelected) {
            for (const studyId of store.state.studies.studiesIds) {
                _setFullSelectedStudy({state}, studyId);
            }
        } else {
            _clearSelection({state});
        }
        _updateBulkSelection({state});
    },
    clearSelection(state) {
        _clearSelection({state});
        _updateBulkSelection({state});
    },
    selectSeries(state, { studyId, seriesId, studySeries, isSelected }) { // completely selects/unselected a series (but not necessarily the parent study)
        const selectedStudy = state.selectedResources[studyId];
        const seriesCount = studySeries.length;

        if (isSelected) {
            if (selectedStudy === undefined) { // the study was not yet selected
                if (seriesCount == 1) { // this is the only series in the study -> full study select
                    _removeStudy({state}, studyId); // remove it in case it was partialy selected
                    _setFullSelectedStudy({state}, studyId);
                } else {
                    _setFullSelectedSeries({state}, studyId, seriesId);
                }
            } else { // the study was already at least partialy selected
                _setFullSelectedSeries({state}, studyId, seriesId);
            }
        } else if (!isSelected && selectedStudy !== undefined) { // we are unselecting a series
            if (selectedStudy['allSeriesSelected']) {
                // this is the first series that we remove from a full selection
                _setPartialySelectedStudy({state}, studyId);
                for (const s of studySeries) {
                    if (s['ID'] != seriesId) {
                        state.selectedResources[studyId]['selectedSeries'][s['ID']] = {
                            'allInstancesSelected': true
                        };
                    }
                }
            } else {
                delete state.selectedResources[studyId]['selectedSeries'][seriesId];
            }
        }
        
        _simplifyAtStudyLevel({state}, studyId, seriesCount);
        _updateBulkSelection({state});
    },
    selectInstance(state, { studyId, seriesId, studySeries, instanceId, seriesInstances, isSelected }) { // completely selects/unselected a series (but not necessarily the parent study)
        const selectedStudy = state.selectedResources[studyId];
        const seriesCount = studySeries.length;
        const instancesCount = seriesInstances.length;

        if (isSelected) {
            if (selectedStudy === undefined) { // the study was not yet selected
                if (seriesCount == 1 && instancesCount == 1) { // this is the only series/instance in the study -> full study select
                    _removeStudy({state}, studyId); // remove it in case it was partialy selected
                    _setFullSelectedStudy({state}, studyId);
                } else {
                    _setPartialySelectedSeries({state}, studyId, seriesId);
                    state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].push(instanceId);
                }
            } else { // the study was already at least partialy selected
                if (!(seriesId in selectedStudy['selectedSeries'])) {
                    _setPartialySelectedSeries({state}, studyId, seriesId);
                }
                state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].push(instanceId);
            }
        } else if (!isSelected && selectedStudy !== undefined) { // we are unselecting an instance
            if (selectedStudy['allSeriesSelected']) {
                // this is the first instance that we remove from a fully selected study
                _setPartialySelectedStudy({state}, studyId);
                for (const s of studySeries) {
                    if (s['ID'] != seriesId) {
                        _setFullSelectedSeries({state}, studyId, s['ID']);
                    }
                }
                if (instancesCount > 1) {
                    _setPartialySelectedSeries({state}, studyId, seriesId);
                    for (const i of seriesInstances) {
                        if (i['ID'] != instanceId) {
                            state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].push(i['ID']);
                        }
                    }
                }
            } else if (selectedStudy['selectedSeries'][seriesId]['allInstancesSelected']) {
                // this is the first instance that we remove from this series
                _setPartialySelectedSeries({state}, studyId, seriesId);
                for (const i of seriesInstances) {
                    if (i['ID'] != instanceId) {
                        state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].push(i['ID']);
                    }
                }

            } else {
                const index = state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].indexOf(instanceId);
                state.selectedResources[studyId]['selectedSeries'][seriesId]['selectedInstances'].splice(index, 1);
            }
        }
        
        _simplifyAtSeriesLevel({state}, studyId, seriesId, instancesCount);
        _simplifyAtStudyLevel({state}, studyId, seriesCount);

        _updateBulkSelection({state});
    }
}

///////////////////////////// ACTIONS

const actions = {
    async selectStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        const isSelected = payload['isSelected'];
        commit('selectStudy', { studyId: studyId, isSelected: isSelected });
    },
    async selectSeries({ commit }, payload) {
        const studyId = payload['studyId'];
        const seriesId = payload['seriesId'];
        const studySeries = payload['studySeries'];
        const isSelected = payload['isSelected'];
        commit('selectSeries', { studyId: studyId, seriesId: seriesId, studySeries: studySeries, isSelected: isSelected });
    },
    async selectInstance({ commit }, payload) {
        const studyId = payload['studyId'];
        const seriesId = payload['seriesId'];
        const studySeries = payload['studySeries'];
        const instanceId = payload['instanceId'];
        const seriesInstances = payload['seriesInstances'];
        const isSelected = payload['isSelected'];
        commit('selectInstance', { studyId: studyId, seriesId: seriesId, studySeries: studySeries, seriesInstances: seriesInstances, instanceId: instanceId, isSelected: isSelected });
    },
    async selectAllStudies({ commit }, payload) {
        const isSelected = payload['isSelected'];
        commit('selectAllStudies', { isSelected: isSelected });
    },
    async deletedStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        commit('removeStudy', { studyId: studyId });
    },
    async clearSelection({ commit }) {
        commit('clearSelection');
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