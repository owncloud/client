import { setWorldConstructor } from 'cucumber';
import { SocketApi } from './socket-api';
import { execSync } from 'child_process';


class CustomWorld {
  constructor({attach, parameters}) {
    this.attach = attach;
    this.parameters = parameters;

    // console.log({parameters});

    this.clientEnv = Object.assign({}, process.env, {
	      HOME: '/tmp/owncloud-gui-test-home',
        XDG_RUNTIME_DIR: '/tmp/owncloud-gui-test-runtime'
    });

    if(this.parameters.spawn_client !== false) {
      // TODO: port to native Node.js
      execSync('mkdir -p ' + this.clientEnv.HOME);
      execSync('rm -rvf ' + this.clientEnv.HOME + '/*');
      execSync('rm -rvf ' + this.clientEnv.HOME + '/.??*');
      execSync('rm -rvf ' + this.clientEnv.XDG_RUNTIME_DIR + '*');

      execSync('mkdir -p ' + this.clientEnv.XDG_RUNTIME_DIR);
    }

    this.socketApi = new SocketApi({world: this});
  }
}

setWorldConstructor(CustomWorld);
