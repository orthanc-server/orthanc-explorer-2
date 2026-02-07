import { createStore, createLogger } from 'vuex'
import configuration from './modules/configuration'
import studies from './modules/studies'
import jobs from './modules/jobs'
import labels from './modules/labels'
import devices from './modules/devices'


const debug = process.env.NODE_ENV !== 'production'

export default createStore({
  modules: {
    studies,
    configuration,
    jobs,
    devices,
    labels
  },
  strict: debug,
  plugins: debug ? [createLogger()] : []
})