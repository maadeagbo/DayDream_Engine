function d = depthBinToJPG(s)
    files = dir(['D:/Docs/kinect_pics/', s,'/depth']);
    for i = 3:length(files)
        filename = files(i).name;

        % read data and do your processing
        % then save with something like:

        infile = ['D:/Docs/kinect_pics/', s, '/depth/',filename]; 
        % read image
        depth = fopen(infile);
        % binary file 
        depth = fread(depth, [512, 424], 'ushort')';
        % flip the depth
        %depth = depth(:, end:-1:1,:);
        % filter
        depth = depth(:,:,1);
        depth(depth>4095) = 4095;
        depth = double(depth);

        str = filename(1:end-4);
        outfile = fullfile('D:/Docs/kinect_pics/', [s, '/depth/', str, '.jpg'])
        imwrite((depth-500)/1500, outfile);
        fclose('all');
    end
end