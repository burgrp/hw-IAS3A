const i2c = require("@device.farm/usb-i2c-driver").open();

const address = 0x72;

//let x = 0;

async function test() {
    try {
        //await i2c.write(address, [x, 1, 2]);
        //x += 2;
        let data = await i2c.read(address, 6);
        console.info(data);
    } catch (e) {
        console.error(e);
    }
    setTimeout(test, 500);
}

test();
