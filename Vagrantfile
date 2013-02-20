Vagrant::Config.run do |config|
  config.vm.define 'linux64' do |target|
    target.vm.box = 'precise'
    target.vm.customize ["modifyvm", :id, "--memory", 2048]
    target.vm.forward_port 22,   2222
    target.vm.network :hostonly, "22.22.22.22"
  end

  config.vm.define 'linux32' do |target|
    target.vm.box = 'precise32'
    target.vm.customize ["modifyvm", :id, "--memory", 2048]
    target.vm.forward_port 22,   2222
    target.vm.network :hostonly, "22.22.22.22"
  end
end
