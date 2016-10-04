% ply
% format ascii 1.0
% element vertex 1280247
% property float x
% property float y
% property float z
% property uchar red
% property uchar green
% property uchar blue
% end_header

function plywrite(vertex, face, rgb, normal, filename)
    fid = fopen(filename, 'w');
    
    fprintf(fid, 'ply\n');
    fprintf(fid, 'format ascii 1.0\n');
    
    fprintf(fid, 'element vertex %d\n', length(vertex(:, 1)));
    fprintf(fid, 'property float x\n');
    fprintf(fid, 'property float y\n');
    fprintf(fid, 'property float z\n');
    fprintf(fid, 'property float nx\n');
    fprintf(fid, 'property float ny\n');
    fprintf(fid, 'property float nz\n');
    fprintf(fid, 'property uchar red\n');
    fprintf(fid, 'property uchar green\n');
    fprintf(fid, 'property uchar blue\n');
    fprintf(fid, 'element face %d\n', length(face(:, 1)));
    fprintf(fid, 'property list uchar int vertex_indices\n');
    fprintf(fid, 'end_header\n');
    
    for i = 1:length(vertex(:, 1))
        fprintf(fid, '%f %f %f %f %f %f %d %d %d\n', ...
            vertex(i, 1), vertex(i, 2), vertex(i, 3), ...
            normal(i, 1), normal(i, 2), normal(i, 3), ...
			rgb(i, 1), rgb(i, 2), rgb(i, 3));
    end
    
    for i = 1:length(face(:, 1))
        fprintf(fid, '3 %d %d %d\n', ...
            face(i, 1)-1, face(i, 2)-1, face(i, 3)-1);
    end
    
    fclose(fid);
end