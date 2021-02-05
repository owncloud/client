import {BeforeAll, defineParameterType, setDefaultTimeout} from 'cucumber';
import { exec } from 'child-process-promise';
import mkdirp from 'mkdirp-promise';

setDefaultTimeout(20 * 1000);

BeforeAll(async function() {
    // TODO: use native Node.js
    return exec('rm -rf test-output/report/*');
});

const codify = require('./helpers/codify')
defineParameterType({
    name: 'code',
    regexp: /"([^"\\]*(\\.[^"\\]*)*)"|'([^'\\]*(\\.[^'\\]*)*)'/,
    type: String,
    transformer: s => codify.replaceInlineCode(s)
})
